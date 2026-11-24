#include "ui.h"

#include "imgui.h"

#include <citro2d.h>

#include <math.h>
#include <string.h>

extern "C" {
extern const unsigned char pophappiness_ttf[];
extern const unsigned int pophappiness_ttf_size;
}

static const ImWchar k_font_ranges[] = {
    0x0020, 0x00FF,
    0x0100, 0x017F,
    0x2010, 0x2027,
    0x2030, 0x203B,
    0x3000, 0x303F,
    0x3040, 0x30FF,
    0x31F0, 0x31FF,
    0xFF01, 0xFFEF,
    0
};

#define ICON_TEX 64
#define ICON_SLOTS 24

struct GpuImage {
    C3D_Tex tex;
    Tex3DS_SubTexture sub;
    C2D_Image img;
    const u16 *pix;
    u32 sum;
    int src_w;
    bool key_zero;
    bool live;
    u32 used;
};

static C3D_RenderTarget *g_top;
static C3D_RenderTarget *g_bot;
static bool g_c3d;
static bool g_c2d;
static bool g_ready;
static bool g_frame;
static bool g_open;
static bool g_touch;
static gfxScreen_t g_which = GFX_TOP;

static ImFontAtlas *g_atlas;
static ImGuiContext *g_ctx_top;
static ImGuiContext *g_ctx_bot;
static GpuImage g_font;
static int g_atlas_w;
static int g_atlas_h;
static int g_pot_w;
static int g_pot_h;
#define KEEP_MAX 40
static u32 *g_keep[KEEP_MAX];
static int g_keep_n;

static void free_keeps(void) {
    for (int i = 0; i < g_keep_n; i++) linearFree(g_keep[i]);
    g_keep_n = 0;
}

static GpuImage g_icons[ICON_SLOTS];
static Tex3DS_SubTexture g_quad_sub;
static C2D_Image g_quad;

static int pot(int n) {
    int p = 8;
    while (p < n) p <<= 1;
    return p;
}

static u32 texel(u8 r, u8 g, u8 b, u8 a) {
    return (u32)a | ((u32)b << 8) | ((u32)g << 16) | ((u32)r << 24);
}

static void unpack565(u16 c, int *r, int *g, int *b) {
    int r5 = (c >> 11) & 31;
    int g6 = (c >> 5) & 63;
    int b5 = c & 31;
    *r = (r5 << 3) | (r5 >> 2);
    *g = (g6 << 2) | (g6 >> 4);
    *b = (b5 << 3) | (b5 >> 2);
}

static u32 checksum(const u16 *pix, int n) {
    u32 sum = 2166136261u;
    int step = n > 48 ? n / 24 : 1;
    if (step < 1) step = 1;
    for (int i = 0; i < n; i += step) sum = (sum ^ pix[i]) * 16777619u;
    return sum ? sum : 1;
}

static void swizzle_rgba8(u32 *dst, const u32 *src, int dim) {
    int tiles = dim >> 3;
    for (int ty = 0; ty < tiles; ty++) {
        for (int tx = 0; tx < tiles; tx++) {
            u32 *tile = dst + ((ty * tiles + tx) << 6);
            const u32 *row = src + ((ty << 3) * dim) + (tx << 3);
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    int m = (x & 1) | ((y & 1) << 1) | ((x & 2) << 1) | ((y & 2) << 2) |
                            ((x & 4) << 2) | ((y & 4) << 3);
                    tile[m] = row[y * dim + x];
                }
            }
        }
    }
}

static bool upload(GpuImage *image, const u32 *rgba, int dim) {
    u32 bytes = (u32)dim * dim * 4;
    u32 *stage = (u32 *)linearAlloc(bytes);
    if (!stage) return false;
    swizzle_rgba8(stage, rgba, dim);
    GSPGPU_FlushDataCache(stage, bytes);
    if (!image->tex.data) {
        if (!C3D_TexInitVRAM(&image->tex, (u16)dim, (u16)dim, GPU_RGBA8)) {
            linearFree(stage);
            return false;
        }
        C3D_TexSetWrap(&image->tex, GPU_CLAMP_TO_EDGE, GPU_CLAMP_TO_EDGE);
        C3D_TexSetFilter(&image->tex, GPU_NEAREST, GPU_NEAREST);
    }
    C3D_TexLoadImage(&image->tex, stage, GPU_TEXFACE_2D, 0);
    if (g_frame) {
        if (g_keep_n < KEEP_MAX) g_keep[g_keep_n++] = stage;
    } else {
        linearFree(stage);
    }
    image->img.tex = &image->tex;
    image->img.subtex = &image->sub;
    image->live = image->tex.data != NULL;
    return image->live;
}

static void apply_style(void) {
    ImGuiStyle &style = ImGui::GetStyle();
    style.WindowPadding = ImVec2(8, 6);
    style.FramePadding = ImVec2(6, 3);
    style.ItemSpacing = ImVec2(8, 4);
    style.ScrollbarSize = 12;
    style.WindowRounding = 0;
    style.FrameRounding = 2;
    style.GrabRounding = 2;
    style.WindowBorderSize = 0;
    style.AntiAliasedLines = false;
    style.AntiAliasedLinesUseTex = false;
    style.AntiAliasedFill = false;
    ImVec4 *c = style.Colors;
    c[ImGuiCol_Text] = ImVec4(0.906f, 0.914f, 0.933f, 1.f);
    c[ImGuiCol_TextDisabled] = ImVec4(0.604f, 0.639f, 0.678f, 1.f);
    c[ImGuiCol_WindowBg] = ImVec4(0.047f, 0.051f, 0.059f, 1.f);
    c[ImGuiCol_ChildBg] = ImVec4(0.047f, 0.051f, 0.059f, 1.f);
    c[ImGuiCol_PopupBg] = ImVec4(0.086f, 0.094f, 0.106f, 1.f);
    c[ImGuiCol_Border] = ImVec4(0.20f, 0.22f, 0.26f, 1.f);
    c[ImGuiCol_FrameBg] = ImVec4(0.086f, 0.094f, 0.106f, 1.f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.12f, 0.14f, 0.17f, 1.f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.16f, 0.20f, 0.26f, 1.f);
    c[ImGuiCol_TitleBg] = ImVec4(0.047f, 0.051f, 0.059f, 1.f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.047f, 0.051f, 0.059f, 1.f);
    c[ImGuiCol_CheckMark] = ImVec4(0.45f, 0.85f, 0.55f, 1.f);
    c[ImGuiCol_Button] = ImVec4(0.14f, 0.18f, 0.24f, 1.f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.18f, 0.26f, 0.38f, 1.f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.22f, 0.34f, 0.52f, 1.f);
    c[ImGuiCol_Header] = ImVec4(0.28f, 0.48f, 0.72f, 1.f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.34f, 0.56f, 0.82f, 1.f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.40f, 0.62f, 0.88f, 1.f);
    c[ImGuiCol_Tab] = ImVec4(0.086f, 0.094f, 0.106f, 1.f);
    c[ImGuiCol_TabHovered] = ImVec4(0.16f, 0.24f, 0.36f, 1.f);
    c[ImGuiCol_TabSelected] = ImVec4(0.18f, 0.32f, 0.52f, 1.f);
    c[ImGuiCol_PlotHistogram] = ImVec4(0.43f, 0.66f, 1.f, 1.f);
    c[ImGuiCol_ScrollbarBg] = ImVec4(0.047f, 0.051f, 0.059f, 1.f);
    c[ImGuiCol_ScrollbarGrab] = ImVec4(0.25f, 0.29f, 0.34f, 1.f);
}

static void prepare_context(ImGuiContext *ctx) {
    ImGui::SetCurrentContext(ctx);
    ImGuiIO &io = ImGui::GetIO();
    io.IniFilename = NULL;
    io.LogFilename = NULL;
    io.DisplaySize = ImVec2(400, 240);
    io.DeltaTime = 1.f / 60.f;
    apply_style();
}

static bool build_font(void) {
    unsigned char *pixels = NULL;
    g_atlas->GetTexDataAsRGBA32(&pixels, &g_atlas_w, &g_atlas_h);
    if (!pixels || g_atlas_w < 1 || g_atlas_h < 1) return false;
    g_pot_w = pot(g_atlas_w);
    g_pot_h = pot(g_atlas_h);
    int dim = g_pot_w > g_pot_h ? g_pot_w : g_pot_h;
    u32 *rgba = (u32 *)linearAlloc((size_t)dim * dim * 4);
    if (!rgba) return false;
    memset(rgba, 0, (size_t)dim * dim * 4);
    const unsigned char *src = pixels;
    for (int y = 0; y < g_atlas_h; y++) {
        u32 *row = rgba + y * dim;
        for (int x = 0; x < g_atlas_w; x++) {
            u8 a = src[(y * g_atlas_w + x) * 4 + 3];
            row[x] = a >= 128 ? texel(255, 255, 255, 255) : 0;
        }
    }
    memset(&g_font, 0, sizeof(g_font));
    bool ok = upload(&g_font, rgba, dim);
    linearFree(rgba);
    g_atlas->ClearTexData();
    if (!ok) return false;
    C3D_TexSetFilter(&g_font.tex, GPU_NEAREST, GPU_NEAREST);
    g_font.sub.left = 0;
    g_font.sub.top = 1;
    g_font.sub.right = 1;
    g_font.sub.bottom = 0;
    g_font.sub.width = (u16)dim;
    g_font.sub.height = (u16)dim;
    g_atlas->SetTexID((ImTextureID)&g_font);
    g_atlas->ClearInputData();
    return true;
}

void ui_init(void) {
    gfxInitDefault();
    gfxSet3D(false);
    gfxSetWide(false);
    if (!C3D_Init(0x80000)) return;
    g_c3d = true;
    if (!C2D_Init(4096)) return;
    g_c2d = true;
    C2D_Prepare();
    C2D_SetTintMode(C2D_TintMult);
    g_top = C2D_CreateScreenTarget(GFX_TOP, GFX_LEFT);
    g_bot = C2D_CreateScreenTarget(GFX_BOTTOM, GFX_LEFT);
    if (!g_top || !g_bot) return;
    g_atlas = IM_NEW(ImFontAtlas)();
    if (!g_atlas) return;
    g_atlas->Flags |= ImFontAtlasFlags_NoMouseCursors | ImFontAtlasFlags_NoBakedLines;
    g_atlas->TexDesiredWidth = 512;
    ImFontConfig font_cfg;
    font_cfg.FontDataOwnedByAtlas = false;
    font_cfg.OversampleH = 1;
    font_cfg.OversampleV = 1;
    font_cfg.PixelSnapH = true;
    if (!g_atlas->AddFontFromMemoryTTF((void *)pophappiness_ttf, (int)pophappiness_ttf_size, 16.0f, &font_cfg, k_font_ranges))
        g_atlas->AddFontDefault();
    g_ctx_top = ImGui::CreateContext(g_atlas);
    g_ctx_bot = ImGui::CreateContext(g_atlas);
    prepare_context(g_ctx_top);
    prepare_context(g_ctx_bot);
    if (!build_font()) return;
    g_ready = true;
}

void ui_drop_images(void) {
    for (int i = 0; i < ICON_SLOTS; i++) {
        if (g_icons[i].live && g_icons[i].tex.data) C3D_TexDelete(&g_icons[i].tex);
        memset(&g_icons[i], 0, sizeof(g_icons[i]));
    }
}

void ui_exit(void) {
    if (g_frame) {
        C3D_FrameEnd(0);
        g_frame = false;
    }
    ui_drop_images();
    if (g_ctx_top) {
        ImGui::DestroyContext(g_ctx_top);
        g_ctx_top = NULL;
    }
    if (g_ctx_bot) {
        ImGui::DestroyContext(g_ctx_bot);
        g_ctx_bot = NULL;
    }
    IM_DELETE(g_atlas);
    g_atlas = NULL;
    if (g_font.tex.data) C3D_TexDelete(&g_font.tex);
    free_keeps();
    if (g_c2d) C2D_Fini();
    if (g_c3d) C3D_Fini();
    gfxExit();
    g_c2d = false;
    g_c3d = false;
    g_ready = false;
}

static u32 g_gen = 1;

static GpuImage *slot_for(const u16 *pix, int src_w, bool key_zero) {
    int free_slot = -1;
    int oldest = -1;
    u32 oldest_used = 0xffffffffu;
    for (int i = 0; i < ICON_SLOTS; i++) {
        GpuImage *slot = &g_icons[i];
        if (!slot->live) {
            if (free_slot < 0) free_slot = i;
            continue;
        }
        if (slot->pix == pix && slot->src_w == src_w && slot->key_zero == key_zero) return slot;
        if (slot->used == g_gen) continue;
        if (slot->used < oldest_used) {
            oldest_used = slot->used;
            oldest = i;
        }
    }
    if (free_slot >= 0) return &g_icons[free_slot];
    if (oldest < 0) return NULL;
    if (g_icons[oldest].tex.data) C3D_TexDelete(&g_icons[oldest].tex);
    memset(&g_icons[oldest], 0, sizeof(g_icons[oldest]));
    return &g_icons[oldest];
}

void *ui_image(const u16 *pix, int src_w, bool key_zero) {
    if (!g_ready || !pix || src_w <= 0 || src_w > ICON_TEX) return NULL;
    u32 sum = checksum(pix, src_w * src_w);
    GpuImage *slot = slot_for(pix, src_w, key_zero);
    if (!slot) return NULL;
    if (!slot->live || slot->sum != sum || slot->pix != pix) {
        u32 *rgba = (u32 *)linearAlloc(ICON_TEX * ICON_TEX * 4);
        if (!rgba) return NULL;
        memset(rgba, 0, ICON_TEX * ICON_TEX * 4);
        for (int row = 0; row < src_w; row++) {
            for (int col = 0; col < src_w; col++) {
                u16 c = pix[row * src_w + col];
                u32 px = 0;
                if (!(key_zero && c == 0)) {
                    int r, g, b;
                    unpack565(c, &r, &g, &b);
                    px = texel((u8)r, (u8)g, (u8)b, 255);
                }
                rgba[row * ICON_TEX + col] = px;
            }
        }
        if (!slot->live) memset(&slot->tex, 0, sizeof(slot->tex));
        if (!upload(slot, rgba, ICON_TEX)) {
            linearFree(rgba);
            return NULL;
        }
        linearFree(rgba);
        float span = src_w / (float)ICON_TEX;
        slot->sub.left = 0;
        slot->sub.right = span;
        slot->sub.top = 1;
        slot->sub.bottom = 1.f - span;
        slot->sub.width = (u16)src_w;
        slot->sub.height = (u16)src_w;
        slot->img.tex = &slot->tex;
        slot->img.subtex = &slot->sub;
        slot->pix = pix;
        slot->src_w = src_w;
        slot->key_zero = key_zero;
        slot->sum = sum;
        slot->live = true;
    }
    slot->used = g_gen;
    return slot;
}

static float g_scissor[4];
static gfxScreen_t g_scissor_screen;
static bool g_scissor_set;

static void set_scissor(float x0, float y0, float x1, float y1) {
    float W = g_which == GFX_TOP ? 400.f : 320.f;
    float H = 240.f;
    if (x0 < 0) x0 = 0;
    if (y0 < 0) y0 = 0;
    if (x1 > W) x1 = W;
    if (y1 > H) y1 = H;
    x0 = floorf(x0);
    y0 = floorf(y0);
    x1 = ceilf(x1);
    y1 = ceilf(y1);
    if (g_scissor_set && g_scissor_screen == g_which && g_scissor[0] == x0 && g_scissor[1] == y0 &&
        g_scissor[2] == x1 && g_scissor[3] == y1)
        return;
    C2D_Flush();
    g_scissor_set = true;
    g_scissor_screen = g_which;
    g_scissor[0] = x0;
    g_scissor[1] = y0;
    g_scissor[2] = x1;
    g_scissor[3] = y1;
    if (x1 <= x0 || y1 <= y0) {
        C3D_SetScissor(GPU_SCISSOR_NORMAL, 0, 0, 0, 0);
        return;
    }
    u32 left = (u32)(H - y1);
    u32 top = (u32)(W - x1);
    u32 right = (u32)(H - y0);
    u32 bottom = (u32)(W - x0);
    if (right <= left) right = left + 1;
    if (bottom <= top) bottom = top + 1;
    C3D_SetScissor(GPU_SCISSOR_NORMAL, left, top, right, bottom);
}

static u32 corner_color(const ImDrawVert *v, int n, float x, float y) {
    const ImDrawVert *best = v;
    float best_d = 1e12f;
    for (int i = 0; i < n; i++) {
        float dx = v[i].pos.x - x;
        float dy = v[i].pos.y - y;
        float d = dx * dx + dy * dy;
        if (d < best_d) {
            best_d = d;
            best = &v[i];
        }
    }
    return best->col;
}

static void draw_textured_quad(GpuImage *image, float x0, float y0, float x1, float y1,
                               float u0, float v0, float u1, float v1, u32 c0, u32 c1, u32 c2, u32 c3) {
    float du = fabsf(u1 - u0) * (image == &g_font ? g_atlas_w : image->src_w);
    float dv = fabsf(v1 - v0) * (image == &g_font ? g_atlas_h : image->src_w);
    if (x1 - x0 < 0.5f || y1 - y0 < 0.5f) return;
    if (image == &g_font && du < 2.f && dv < 2.f) {
        C2D_DrawRectangle(x0, y0, 0, x1 - x0, y1 - y0, c0, c1, c2, c3);
        return;
    }
    if (image != &g_font) {
        g_quad.tex = image->img.tex;
        g_quad.subtex = image->img.subtex;
    } else {
        int dim = g_font.sub.width > 0 ? g_font.sub.width : 1;
        float us = g_atlas_w / (float)dim;
        float vs = g_atlas_h / (float)dim;
        if (u0 > u1) {
            float t = u0;
            u0 = u1;
            u1 = t;
        }
        if (v0 > v1) {
            float t = v0;
            v0 = v1;
            v1 = t;
        }
        g_quad_sub.left = u0 * us;
        g_quad_sub.right = u1 * us;
        g_quad_sub.top = 1.f - v0 * vs;
        g_quad_sub.bottom = 1.f - v1 * vs;
        if (g_quad_sub.top < g_quad_sub.bottom) {
            float t = g_quad_sub.top;
            g_quad_sub.top = g_quad_sub.bottom;
            g_quad_sub.bottom = t;
        }
        g_quad_sub.width = 1;
        g_quad_sub.height = 1;
        g_quad.tex = &g_font.tex;
        g_quad.subtex = &g_quad_sub;
    }
    C2D_ImageTint tint;
    C2D_SetImageTint(&tint, C2D_TopLeft, c0, 1.f);
    C2D_SetImageTint(&tint, C2D_TopRight, c1, 1.f);
    C2D_SetImageTint(&tint, C2D_BotLeft, c2, 1.f);
    C2D_SetImageTint(&tint, C2D_BotRight, c3, 1.f);
    C2D_DrawParams params;
    params.pos.x = x0;
    params.pos.y = y0;
    params.pos.w = x1 - x0;
    params.pos.h = y1 - y0;
    params.center.x = 0;
    params.center.y = 0;
    params.depth = 0;
    params.angle = 0;
    C2D_DrawImage(g_quad, &params, &tint);
}

static bool emit_quad(const ImDrawVert *vtx, const ImDrawIdx *idx, GpuImage *image) {
    int id[6];
    int uniq[4];
    int n = 0;
    for (int i = 0; i < 6; i++) id[i] = (int)idx[i];
    for (int i = 0; i < 6; i++) {
        bool seen = false;
        for (int j = 0; j < n; j++) {
            if (uniq[j] == id[i]) seen = true;
        }
        if (seen) continue;
        if (n == 4) return false;
        uniq[n++] = id[i];
    }
    if (n != 4) return false;
    ImDrawVert v[4];
    for (int i = 0; i < 4; i++) v[i] = vtx[uniq[i]];
    float minx = v[0].pos.x, maxx = v[0].pos.x, miny = v[0].pos.y, maxy = v[0].pos.y;
    float minu = v[0].uv.x, maxu = v[0].uv.x, minv = v[0].uv.y, maxv = v[0].uv.y;
    for (int i = 1; i < 4; i++) {
        if (v[i].pos.x < minx) minx = v[i].pos.x;
        if (v[i].pos.x > maxx) maxx = v[i].pos.x;
        if (v[i].pos.y < miny) miny = v[i].pos.y;
        if (v[i].pos.y > maxy) maxy = v[i].pos.y;
        if (v[i].uv.x < minu) minu = v[i].uv.x;
        if (v[i].uv.x > maxu) maxu = v[i].uv.x;
        if (v[i].uv.y < minv) minv = v[i].uv.y;
        if (v[i].uv.y > maxv) maxv = v[i].uv.y;
    }
    for (int i = 0; i < 4; i++) {
        bool xok = fabsf(v[i].pos.x - minx) < 0.75f || fabsf(v[i].pos.x - maxx) < 0.75f;
        bool yok = fabsf(v[i].pos.y - miny) < 0.75f || fabsf(v[i].pos.y - maxy) < 0.75f;
        if (!xok || !yok) return false;
    }
    u32 c0;
    u32 c1;
    u32 c2;
    u32 c3;
    if (v[0].col == v[1].col && v[0].col == v[2].col && v[0].col == v[3].col) {
        c0 = c1 = c2 = c3 = v[0].col;
    } else {
        c0 = corner_color(v, 4, minx, miny);
        c1 = corner_color(v, 4, maxx, miny);
        c2 = corner_color(v, 4, minx, maxy);
        c3 = corner_color(v, 4, maxx, maxy);
    }
    draw_textured_quad(image, minx, miny, maxx, maxy, minu, minv, maxu, maxv, c0, c1, c2, c3);
    return true;
}

static void render_draw_data(ImDrawData *draw) {
    if (!draw) return;
    float W = g_which == GFX_TOP ? 400.f : 320.f;
    for (int n = 0; n < draw->CmdListsCount; n++) {
        const ImDrawList *list = draw->CmdLists[n];
        const ImDrawVert *vtx = list->VtxBuffer.Data;
        const ImDrawIdx *idx = list->IdxBuffer.Data;
        for (int c = 0; c < list->CmdBuffer.Size; c++) {
            const ImDrawCmd &cmd = list->CmdBuffer[c];
            if (cmd.UserCallback) continue;
            if (cmd.ElemCount == 0) continue;
            ImVec4 clip = cmd.ClipRect;
            clip.x -= draw->DisplayPos.x;
            clip.y -= draw->DisplayPos.y;
            clip.z -= draw->DisplayPos.x;
            clip.w -= draw->DisplayPos.y;
            if (clip.x >= W || clip.y >= 240.f || clip.z <= 0 || clip.w <= 0) continue;
            set_scissor(clip.x, clip.y, clip.z, clip.w);
            GpuImage *image = (GpuImage *)cmd.GetTexID();
            if (!image) image = &g_font;
            const ImDrawVert *base = vtx + cmd.VtxOffset;
            const ImDrawIdx *ib = idx + cmd.IdxOffset;
            for (unsigned i = 0; i < cmd.ElemCount;) {
                if (i + 6 <= cmd.ElemCount && emit_quad(base, ib + i, image)) {
                    i += 6;
                    continue;
                }
                if (i + 3 > cmd.ElemCount) break;
                const ImDrawVert &a = base[ib[i]];
                const ImDrawVert &b = base[ib[i + 1]];
                const ImDrawVert &d = base[ib[i + 2]];
                C2D_DrawTriangle(a.pos.x, a.pos.y, a.col, b.pos.x, b.pos.y, b.col, d.pos.x, d.pos.y, d.col, 0);
                i += 3;
            }
        }
    }
    C2D_Flush();
    C3D_SetScissor(GPU_SCISSOR_DISABLE, 0, 0, 0, 0);
}

static void flush_screen(void) {
    if (!g_open) return;
    ImGui::Render();
    render_draw_data(ImGui::GetDrawData());
    g_open = false;
}

static void feed_mouse(ImGuiIO &io) {
    if (g_which != GFX_BOTTOM) {
        io.AddMousePosEvent(-10000.f, -10000.f);
        return;
    }
    if (hidKeysHeld() & KEY_TOUCH) {
        touchPosition touch;
        hidTouchRead(&touch);
        io.AddMousePosEvent((float)touch.px, (float)touch.py);
        if (!g_touch) io.AddMouseButtonEvent(0, true);
        g_touch = true;
    } else {
        if (g_touch) io.AddMouseButtonEvent(0, false);
        g_touch = false;
        io.AddMousePosEvent(-10000.f, -10000.f);
    }
}

void ui_screen(gfxScreen_t screen) {
    if (!g_ready) return;
    if (!g_frame) {
        C3D_FrameBegin(C3D_FRAME_SYNCDRAW);
        free_keeps();
        C2D_TargetClear(g_top, C2D_Color32(12, 13, 15, 255));
        C2D_TargetClear(g_bot, C2D_Color32(12, 13, 15, 255));
        g_frame = true;
    }
    if (g_open) flush_screen();
    g_which = screen;
    g_scissor_set = false;
    C2D_SceneBegin(screen == GFX_TOP ? g_top : g_bot);
    ImGui::SetCurrentContext(screen == GFX_TOP ? g_ctx_top : g_ctx_bot);
    ImGuiIO &io = ImGui::GetIO();
    io.DisplaySize = ImVec2(screen == GFX_TOP ? 400.f : 320.f, 240.f);
    io.DeltaTime = 1.f / 60.f;
    feed_mouse(io);
    ImGui::NewFrame();
    g_open = true;
}

void ui_present(void) {
    if (!g_ready) return;
    if (g_open) flush_screen();
    if (!g_frame) return;
    C3D_FrameEnd(0);
    g_frame = false;
    if (++g_gen == 0) g_gen = 1;
}
