#include "titles.h"
#include "cia.h"
#include "ui.h"
#include "bgm.h"
#include "version.h"

#include "imgui.h"

#include <3ds/result.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define LIST_HEADER 18
#define ROW_H 52
#define LIST_ROWS 4
#define GRID_COLS 4
#define GRID_STRIDE 52.f
#define GROUP_H 22.f
#define CIA_ROW_H 52
#define CIA_ROWS 4

typedef enum {
    MODE_LIST = 0,
    MODE_CONFIRM,
    MODE_RESULT,
    MODE_INSTALL,
    MODE_INSTALL_CONFIRM,
    MODE_REINSTALL
} Mode;

static int g_visible[TITLE_CAP];
static u64 g_ids[TITLE_CAP];
static u8 g_replace[CIA_CAP];

static bool already_installed(Result rc) {
    return rc == (Result)0xC8E083FC || (rc & 0x3FFu) == RD_ALREADY_EXISTS;
}

static const char *explain_result(Result rc) {
    if (rc == (Result)0xC8804471) return "The SD card could not be opened.";
    if (rc == (Result)0xC86044D2 || rc == CIA_ERR_OOM) return "Not enough memory.";
    if (rc == CIA_ERR_TRUNCATED) return "That CIA file is incomplete.";
    if (rc == CIA_ERR_SYSTEM) return "System software is left alone.";
    if (rc == CIA_ERR_CANCEL) return "Cancelled.";
    if (rc == (Result)0xD8E0806A || ((rc & 0x3FFu) == 0x6A && ((rc >> 10) & 0xFFu) == RM_AM))
        return "That CIA was rejected. Boot Luma3DS from the Select menu. If you already did, the file is damaged.";

    u32 module = (rc >> 10) & 0xFFu;
    u32 desc = rc & 0x3FFu;
    u32 summary = (rc >> 21) & 0x3Fu;
    if (desc == RD_OUT_OF_MEMORY || summary == RS_OUTOFRESOURCE || desc == RD_TOO_LARGE)
        return "Not enough free space.";
    if (desc == RD_ALREADY_EXISTS) return "That title is already installed.";
    if (desc == RD_NOT_FOUND || summary == RS_NOTFOUND) {
        if (module == RM_SRV)
            return "Install Cinder with FBI, then open its icon on the HOME Menu.";
        if (module == RM_FS || module == RM_SDMC) return "That file is not on the SD card.";
        return "That title is no longer installed.";
    }
    if (desc == RD_NOT_AUTHORIZED)
        return "Install Cinder with FBI, then open its icon on the HOME Menu.";
    if (desc == RD_CANCEL_REQUESTED || summary == RS_CANCELED) return "Cancelled.";
    if (desc == RD_BUSY) return "The system could not change that title.";
    if (desc == RD_NOT_INITIALIZED || summary == RS_INVALIDSTATE)
        return "The system was not ready. Reboot and try again.";
    if (summary == RS_NOTSUPPORTED || desc == RD_NOT_IMPLEMENTED) return "The system does not allow that.";
    if (desc == RD_INVALID_SIZE || desc == RD_MISALIGNED_SIZE) return "That file looks damaged.";
    if (module == RM_FS || module == RM_SDMC || module == RM_FATFS)
        return "The SD card could not be read. Check that it is inserted.";
    if (module == RM_AM || module == RM_AM_LOW)
        return "The system could not change that title.";
    return "Something went wrong. Reboot and try again.";
}

static bool is_nds(const TitleRec *title) {
    return title->kind == KIND_DSIWARE;
}

static u32 g_shelf_gen;

static int group_rank(const TitleRec *title) {
    if (!title->licensed) return 3;
    if (title->kind == KIND_UPDATE) return 1;
    if (title->kind == KIND_DLC) return 2;
    return 0;
}

static const TitleList *g_sort_list;

static int cmp_visible(const void *a, const void *b) {
    const TitleRec *ta = &g_sort_list->items[*(const int *)a];
    const TitleRec *tb = &g_sort_list->items[*(const int *)b];
    int ra = group_rank(ta);
    int rb = group_rank(tb);
    if (ra != rb) return ra - rb;
    if (ta->size < tb->size) return 1;
    if (ta->size > tb->size) return -1;
    return strcmp(ta->name, tb->name);
}

static int build_visible(const TitleList *list, int nds, int *out) {
    static u32 built_gen;
    static int built_nds = -1;
    static int built_n = -1;
    static const TitleList *built_list;
    if (built_list == list && built_nds == nds && built_gen == g_shelf_gen && built_n >= 0) return built_n;
    int count = 0;
    for (u32 i = 0; i < list->count; i++) {
        const TitleRec *title = &list->items[i];
        if (title->kind == KIND_SYSTEM) continue;
        if (title->id == CINDER_TITLE_ID || title->id == titles_self_id() || titles_hidden(title->id)) continue;
        if (nds) {
            if (is_nds(title)) out[count++] = (int)i;
        } else if (!is_nds(title)) {
            out[count++] = (int)i;
        }
    }
    if (!nds && count > 1) {
        g_sort_list = list;
        qsort(out, (size_t)count, sizeof(int), cmp_visible);
    }
    built_list = list;
    built_nds = nds;
    built_n = count;
    built_gen = g_shelf_gen;
    return count;
}

static void reload(TitleList *list, Result *load_rc, bool after_change);
static void draw_loading(void);
static bool g_loading;

static int utf8_advance(const char *s) {
    unsigned char c = (unsigned char)s[0];
    if (c == 0) return 0;
    if (c < 0x80) return 1;
    if ((c & 0xE0) == 0xC0 && (s[1] & 0xC0) == 0x80) return 2;
    if ((c & 0xF0) == 0xE0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80) return 3;
    if ((c & 0xF8) == 0xF0 && (s[1] & 0xC0) == 0x80 && (s[2] & 0xC0) == 0x80 && (s[3] & 0xC0) == 0x80) return 4;
    return 1;
}

static void clip_chars(char *dst, size_t cap, const char *src, int limit) {
    int total = 0;
    const char *scan = src ? src : "";
    while (*scan) {
        int a = utf8_advance(scan);
        if (a <= 0) break;
        scan += a;
        total++;
    }
    if (!src) src = "";
    if (total <= limit) {
        snprintf(dst, cap, "%s", src);
        return;
    }
    size_t o = 0;
    int n = 0;
    while (*src && n < limit && o + 1 < cap) {
        int a = utf8_advance(src);
        if (a <= 0 || o + (size_t)a >= cap) break;
        memcpy(dst + o, src, (size_t)a);
        o += (size_t)a;
        src += a;
        n++;
    }
    if (o + 3 < cap) {
        dst[o++] = '.';
        dst[o++] = '.';
        dst[o++] = '.';
    }
    dst[o] = '\0';
}

static void wrap_name(char *dst, size_t cap, const char *src, int limit) {
    int total = 0;
    for (const char *p = src; *p;) {
        int a = utf8_advance(p);
        if (a <= 0) break;
        p += a;
        total++;
    }
    if (total <= limit) {
        snprintf(dst, cap, "%s", src);
        return;
    }
    size_t o = 0;
    int col = 0;
    int last_space = -1;
    const char *p = src;
    while (*p && o + 1 < cap) {
        int a = utf8_advance(p);
        unsigned char c = (unsigned char)p[0];
        if (c == '\n') {
            dst[o++] = '\n';
            col = 0;
            last_space = -1;
            p += a;
            continue;
        }
        if (col >= limit) {
            if (last_space >= 0) {
                dst[last_space] = '\n';
                col = 0;
                for (size_t i = (size_t)last_space + 1; i < o;) {
                    int n = utf8_advance(dst + i);
                    if (n <= 0) break;
                    i += (size_t)n;
                    col++;
                }
                last_space = -1;
            } else {
                dst[o++] = '\n';
                if (o + 1 >= cap) break;
                col = 0;
            }
        }
        if (c == ' ' && col == 0) {
            p += a;
            continue;
        }
        if (o + (size_t)a >= cap) break;
        if (c == ' ') last_space = (int)o;
        memcpy(dst + o, p, (size_t)a);
        o += (size_t)a;
        p += a;
        col++;
    }
    dst[o] = '\0';
}

static void load_pump(void) {
    if (g_loading) draw_loading();
}

static void reload(TitleList *list, Result *load_rc, bool after_change) {
    g_loading = after_change;
    if (after_change) draw_loading();
    ui_drop_images();
    titles_free(list);
    *load_rc = titles_load_all(list);
    g_shelf_gen++;
    g_loading = false;
}

static u32 count_selected(const TitleList *list, u64 *bytes) {
    u32 count = 0;
    *bytes = 0;
    for (u32 i = 0; i < list->count; i++) {
        if (list->items[i].selected && list->items[i].deletable) {
            count++;
            *bytes += list->items[i].size;
        }
    }
    return count;
}

static void select_visible(TitleList *list, const int *visible, int count, bool on) {
    for (int i = 0; i < count; i++) {
        TitleRec *title = &list->items[visible[i]];
        if (title->deletable) title->selected = on;
    }
}

static void toggle_visible(TitleList *list, const int *visible, int count) {
    int selectable = 0;
    int selected = 0;
    for (int i = 0; i < count; i++) {
        TitleRec *title = &list->items[visible[i]];
        if (!title->deletable) continue;
        selectable++;
        if (title->selected) selected++;
    }
    if (selectable == 0) return;
    select_visible(list, visible, count, selected < selectable);
}

static bool repeat_key(u32 down, u32 held, u32 key, int *timer) {
    if (down & key) {
        *timer = 0;
        return true;
    }
    if (held & key) {
        (*timer)++;
        return *timer > 16 && (*timer % 3) == 0;
    }
    *timer = 0;
    return false;
}

static FS_MediaType media_of(const TitleList *list, u64 id) {
    for (u32 i = 0; i < list->count; i++) {
        if (list->items[i].id == id) return list->items[i].media;
    }
    return MEDIATYPE_SD;
}

static void panel(const char *id) {
    ImGui::SetNextWindowPos(ImVec2(0, 0));
    ImGui::SetNextWindowSize(ImGui::GetIO().DisplaySize);
    ImGui::Begin(id, NULL,
                 ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoMove |
                     ImGuiWindowFlags_NoSavedSettings | ImGuiWindowFlags_NoBringToFrontOnFocus |
                     ImGuiWindowFlags_NoNav);
}

static void *icon_tex(const TitleRec *title, u32 tick) {
    const u16 *pix = title->icon;
    int src = title->icon_w > 0 ? title->icon_w : 48;
    if (title->anim) {
        u16 token = titles_anim_token(title->anim, tick);
        if (token) {
            struct Frame {
                const TitleRec *title;
                u16 token;
                u32 used;
                u16 pix[32 * 32];
            };
            static Frame cache[16];
            static u32 stamp = 1;
            int slot = -1;
            int oldest = 0;
            for (int i = 0; i < 16; i++) {
                if (cache[i].title == title) slot = i;
                if (cache[i].used < cache[oldest].used) oldest = i;
            }
            if (slot < 0) slot = oldest;
            if (cache[slot].title != title || cache[slot].token != token) {
                if (titles_anim_frame(title->anim, tick, cache[slot].pix)) {
                    cache[slot].title = title;
                    cache[slot].token = token;
                }
            }
            if (cache[slot].title == title) {
                pix = cache[slot].pix;
                src = 32;
                cache[slot].used = stamp++;
            }
        }
    }
    if (!pix) return NULL;
    return ui_image(pix, src, title->kind == KIND_DSIWARE);
}

static void storage_line(char *out, size_t n, const char *name, Result rc, const FS_ArchiveResource *res) {
    if (R_FAILED(rc) || res->totalClusters == 0) {
        snprintf(out, n, "%s  --", name);
        return;
    }
    u64 cluster = res->clusterSize ? (u64)res->clusterSize : 1;
    char free_s[16];
    char total_s[16];
    titles_format_size(free_s, sizeof(free_s), (u64)res->freeClusters * cluster);
    titles_format_size(total_s, sizeof(total_s), (u64)res->totalClusters * cluster);
    snprintf(out, n, "%s  %s / %s", name, free_s, total_s);
}

static bool g_ptmu;

static void civil_date(u32 days, int *year, int *month, int *day) {
    int y = 1900;
    while (1) {
        int leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
        u32 diy = leap ? 366u : 365u;
        if (days < diy) break;
        days -= diy;
        y++;
    }
    static const u8 mdays[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
    int leap = (y % 4 == 0 && y % 100 != 0) || y % 400 == 0;
    int m = 0;
    for (; m < 12; m++) {
        u32 dim = mdays[m] + (m == 1 && leap);
        if (days < dim) break;
        days -= dim;
    }
    *year = y;
    *month = m + 1;
    *day = (int)days + 1;
}

static void format_clock(char *out, size_t n) {
    u64 secs = osGetTime() / 1000;
    u32 days = (u32)(secs / 86400);
    u32 sod = (u32)(secs % 86400);
    int hour = (int)(sod / 3600);
    int minute = (int)((sod % 3600) / 60);
    int year = 0;
    int month = 1;
    int day = 1;
    civil_date(days, &year, &month, &day);
    static const char *mo[] = {"Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"};
    int h12 = hour % 12;
    if (h12 == 0) h12 = 12;
    snprintf(out, n, "%s %d %d  %d:%02d %s", mo[month - 1], day, year, h12, minute, hour < 12 ? "AM" : "PM");
}

static void draw_battery(void) {
    static u64 stamp;
    static u8 level;
    static bool charging;
    u64 now = osGetTime();
    if (stamp == 0 || now - stamp >= 2000) {
        u8 raw = 0;
        u8 charge = 0;
        level = 0;
        charging = false;
        if (g_ptmu && R_SUCCEEDED(PTMU_GetBatteryLevel(&raw))) {
            if (raw > 5) raw = 5;
            level = raw;
        }
        if (g_ptmu && R_SUCCEEDED(PTMU_GetBatteryChargeState(&charge))) charging = charge != 0;
        stamp = now;
    }
    ImVec2 p = ImGui::GetCursorScreenPos();
    float h = ImGui::GetTextLineHeight();
    float w = 18.f;
    float bh = h * 0.62f;
    float by = p.y + (h - bh) * 0.5f;
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImU32 frame = IM_COL32(231, 233, 238, 255);
    ImU32 fill = charging ? IM_COL32(115, 217, 140, 255) : frame;
    if (!charging && level <= 1) fill = IM_COL32(181, 82, 74, 255);
    dl->AddRect(ImVec2(p.x, by), ImVec2(p.x + w - 3.f, by + bh), frame, 1.f);
    dl->AddRectFilled(ImVec2(p.x + w - 3.f, by + bh * 0.28f), ImVec2(p.x + w, by + bh * 0.72f), frame);
    if (level > 0) {
        float inner = (w - 7.f) * ((float)level / 5.f);
        dl->AddRectFilled(ImVec2(p.x + 2.f, by + 2.f), ImVec2(p.x + 2.f + inner, by + bh - 2.f), fill);
    }
    ImGui::Dummy(ImVec2(w + 2.f, h));
}

static void draw_clock_line(void) {
    char left[24];
    char clock[40];
    snprintf(left, sizeof(left), "Cinder %s", CINDER_VERSION);
    format_clock(clock, sizeof(clock));
    const float batt = 22.f;
    float pad = ImGui::GetStyle().WindowPadding.x;
    float left_w = ImGui::CalcTextSize(left).x;
    float clock_w = ImGui::CalcTextSize(clock).x;
    float x = ImGui::GetWindowWidth() - pad - batt - 8.f - clock_w;
    if (x < left_w + 12.f) {
        u64 secs = osGetTime() / 1000;
        u32 sod = (u32)(secs % 86400);
        int hour = (int)(sod / 3600);
        int minute = (int)((sod % 3600) / 60);
        int h12 = hour % 12;
        if (h12 == 0) h12 = 12;
        snprintf(clock, sizeof(clock), "%d:%02d %s", h12, minute, hour < 12 ? "AM" : "PM");
        clock_w = ImGui::CalcTextSize(clock).x;
        x = ImGui::GetWindowWidth() - pad - batt - 8.f - clock_w;
    }
    ImGui::TextUnformatted(left);
    ImGui::SameLine();
    ImGui::SetCursorPosX(x);
    ImGui::TextUnformatted(clock);
    ImGui::SameLine(0.f, 8.f);
    draw_battery();
}

static void draw_loading(void) {
    ui_screen(GFX_TOP);
    panel("loading");
    draw_clock_line();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.f, 48.f));
    ImGui::Text("Loading titles...");
    ImGui::End();
    ui_screen(GFX_BOTTOM);
    panel("loading-b");
    ImGui::Dummy(ImVec2(0.f, 88.f));
    ImGui::Text("Loading titles...");
    ImGui::End();
    ui_present();
}

static float key_width(int kind, float h) {
    if (kind == 1 || kind == 2 || kind == 6) return h * 0.92f;
    if (kind == 3) return ImGui::CalcTextSize("SELECT").x + h * 0.55f;
    if (kind == 4) return ImGui::CalcTextSize("START").x + h * 0.55f;
    if (kind == 5) return key_width(4, h) + h * 0.45f + key_width(3, h);
    return 0.f;
}

static float draw_face(ImDrawList *dl, float x, float y, float h, const char *letter) {
    float d = h * 0.92f;
    float r = d * 0.5f;
    ImVec2 c(x + r, y + h * 0.5f);
    dl->AddCircleFilled(c, r, IM_COL32(22, 24, 28, 255), 20);
    dl->AddCircle(c, r, IM_COL32(197, 206, 216, 255), 20, 1.15f);
    ImVec2 ts = ImGui::CalcTextSize(letter);
    dl->AddText(ImVec2(c.x - ts.x * 0.5f, c.y - ts.y * 0.5f), IM_COL32(231, 233, 238, 255), letter);
    return d;
}

static float draw_pill(ImDrawList *dl, float x, float y, float h, const char *word) {
    ImVec2 ts = ImGui::CalcTextSize(word);
    float w = ts.x + h * 0.55f;
    float bh = h * 0.82f;
    float by = y + (h - bh) * 0.5f;
    ImVec2 a(x, by);
    ImVec2 b(x + w, by + bh);
    dl->AddRectFilled(a, b, IM_COL32(22, 24, 28, 255), bh * 0.5f);
    dl->AddRect(a, b, IM_COL32(197, 206, 216, 255), bh * 0.5f, 0, 1.f);
    dl->AddText(ImVec2(x + (w - ts.x) * 0.5f, y + (h - ts.y) * 0.5f), IM_COL32(231, 233, 238, 255), word);
    return w;
}

static float draw_key(ImDrawList *dl, float x, float y, float h, int kind) {
    if (kind == 1) return draw_face(dl, x, y, h, "A");
    if (kind == 2) return draw_face(dl, x, y, h, "X");
    if (kind == 6) return draw_face(dl, x, y, h, "Y");
    if (kind == 3) return draw_pill(dl, x, y, h, "SELECT");
    if (kind == 4) return draw_pill(dl, x, y, h, "START");
    if (kind == 5) {
        float a = draw_pill(dl, x, y, h, "START");
        float gap = h * 0.45f;
        ImVec2 ts = ImGui::CalcTextSize("+");
        dl->AddText(ImVec2(x + a + (gap - ts.x) * 0.5f, y + (h - ts.y) * 0.5f), IM_COL32(231, 233, 238, 255), "+");
        float b = draw_pill(dl, x + a + gap, y, h, "SELECT");
        return a + gap + b;
    }
    return 0.f;
}

static void draw_prompts(const int *keys, const char *const *actions, int count) {
    float avail = ImGui::GetWindowContentRegionMax().x - ImGui::GetWindowContentRegionMin().x;
    float natural = ImGui::GetTextLineHeight();
    float width = 0.f;
    for (int i = 0; i < count; i++) {
        width += key_width(keys[i], natural);
        width += 3.f + ImGui::CalcTextSize(actions[i]).x;
        if (i + 1 < count) width += 8.f;
    }
    float scale = 1.f;
    if (width > avail && width > 1.f) scale = avail / width;
    ImGui::SetWindowFontScale(scale);
    float h = ImGui::GetTextLineHeight();
    float y = ImGui::GetWindowContentRegionMax().y - h;
    ImGui::SetCursorPosY(y);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImDrawList *dl = ImGui::GetWindowDrawList();
    float x = origin.x;
    for (int i = 0; i < count; i++) {
        x += draw_key(dl, x, origin.y, h, keys[i]);
        x += 3.f * scale;
        ImGui::SetCursorScreenPos(ImVec2(x, origin.y));
        ImGui::TextDisabled("%s", actions[i]);
        x = ImGui::GetItemRectMax().x + 8.f * scale;
    }
    ImGui::SetWindowFontScale(1.f);
}

static void draw_selection(u32 selected, u64 bytes, float inset) {
    char size[16];
    char line[40];
    titles_format_size(size, sizeof(size), bytes);
    snprintf(line, sizeof(line), "%lu selected  %s", (unsigned long)selected, size);
    float w = ImGui::CalcTextSize(line).x;
    ImGui::SameLine();
    ImGui::SetCursorPosX(ImGui::GetWindowWidth() - w - ImGui::GetStyle().WindowPadding.x - inset);
    ImGui::TextUnformatted(line);
}

static void storage_side(float y) {
    static u64 stamp;
    static char a[64];
    static char b[64];
    u64 now = osGetTime();
    if (a[0] == '\0' || now - stamp >= 2000) {
        FS_ArchiveResource sd;
        FS_ArchiveResource nand;
        memset(&sd, 0, sizeof(sd));
        memset(&nand, 0, sizeof(nand));
        Result sd_rc = FSUSER_GetSdmcArchiveResource(&sd);
        Result nand_rc = FSUSER_GetNandArchiveResource(&nand);
        storage_line(a, sizeof(a), "SD", sd_rc, &sd);
        storage_line(b, sizeof(b), "NAND", nand_rc, &nand);
        stamp = now;
    }
    float right = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x;
    float step = ImGui::GetTextLineHeightWithSpacing();
    ImVec2 saved = ImGui::GetCursorScreenPos();
    ImGui::SetCursorScreenPos(ImVec2(right - ImGui::CalcTextSize(a).x, y));
    ImGui::TextDisabled("%s", a);
    ImGui::SetCursorScreenPos(ImVec2(right - ImGui::CalcTextSize(b).x, y + step));
    ImGui::TextDisabled("%s", b);
    ImGui::SetCursorScreenPos(saved);
}

static int g_shelf = -1;
static bool g_grid;
static bool g_ate_click;

static void nudge(int *cursor, int count, int dx, int dy) {
    if (count <= 0) return;
    if (*cursor < 0) *cursor = 0;
    if (*cursor >= count) *cursor = count - 1;
    if (!g_grid) {
        if (dy < 0 && *cursor > 0) *cursor -= 1;
        if (dy > 0 && *cursor + 1 < count) *cursor += 1;
        return;
    }
    if (dx < 0 && *cursor > 0) *cursor -= 1;
    if (dx > 0 && *cursor + 1 < count) *cursor += 1;
    if (dy < 0 && *cursor >= GRID_COLS) *cursor -= GRID_COLS;
    if (dy > 0 && *cursor + GRID_COLS < count) *cursor += GRID_COLS;
}

static int group_span(const TitleList *list, const int *visible, int vcount, int rank, int *start) {
    int s = 0;
    while (s < vcount && group_rank(&list->items[visible[s]]) < rank) s++;
    int e = s;
    while (e < vcount && group_rank(&list->items[visible[e]]) == rank) e++;
    *start = s;
    return e - s;
}

static void nudge_grouped(const TitleList *list, const int *visible, int count, int *cursor, int dx, int dy) {
    if (count <= 0) return;
    if (*cursor < 0) *cursor = 0;
    if (*cursor >= count) *cursor = count - 1;
    int rank = group_rank(&list->items[visible[*cursor]]);
    int start = 0;
    int n = group_span(list, visible, count, rank, &start);
    if (n <= 0) return;
    int local = *cursor - start;
    int col = local % GRID_COLS;
    int row = local / GRID_COLS;
    int rows = (n + GRID_COLS - 1) / GRID_COLS;
    if (dx < 0 && local > 0) local -= 1;
    if (dx > 0 && local + 1 < n) local += 1;
    if (dy < 0) {
        if (row > 0) local -= GRID_COLS;
        else {
            for (int g = rank - 1; g >= 0; g--) {
                int ps = 0;
                int pn = group_span(list, visible, count, g, &ps);
                if (pn <= 0) continue;
                int pr = (pn - 1) / GRID_COLS;
                int idx = pr * GRID_COLS + col;
                if (idx >= pn) idx = pn - 1;
                *cursor = ps + idx;
                return;
            }
        }
    }
    if (dy > 0) {
        if (row + 1 < rows) {
            int idx = local + GRID_COLS;
            if (idx >= n) idx = n - 1;
            local = idx;
        } else {
            for (int g = rank + 1; g <= 3; g++) {
                int ns = 0;
                int nn = group_span(list, visible, count, g, &ns);
                if (nn <= 0) continue;
                int idx = col < nn ? col : nn - 1;
                *cursor = ns + idx;
                return;
            }
        }
    }
    *cursor = start + local;
}

static bool corner_view(void) {
    const float s = 18.f;
    ImVec2 saved = ImGui::GetCursorScreenPos();
    ImVec2 win = ImGui::GetWindowPos();
    ImVec2 max = ImGui::GetWindowContentRegionMax();
    ImGui::SetCursorScreenPos(ImVec2(win.x + max.x - s, win.y + 5.f));
    ImGui::PushID("viewmode");
    bool hit = ImGui::InvisibleButton("v", ImVec2(s, s));
    ImDrawList *dl = ImGui::GetWindowDrawList();
    ImVec2 a = ImGui::GetItemRectMin();
    ImU32 col = ImGui::IsItemHovered() ? IM_COL32(231, 233, 238, 255) : IM_COL32(154, 163, 173, 255);
    if (g_grid) {
        for (int i = 0; i < 3; i++) {
            float y = a.y + 1.f + (float)i * 6.f;
            dl->AddRectFilled(ImVec2(a.x, y), ImVec2(a.x + s, y + 2.2f), col, 1.f);
        }
    } else {
        float g = 2.f;
        float c = (s - g) * 0.5f;
        for (int y = 0; y < 2; y++) {
            for (int x = 0; x < 2; x++) {
                ImVec2 o(a.x + (float)x * (c + g), a.y + (float)y * (c + g));
                dl->AddRectFilled(o, ImVec2(o.x + c, o.y + c), col, 1.f);
            }
        }
    }
    ImGui::PopID();
    ImGui::SetCursorScreenPos(saved);
    return hit;
}

static void shelf_tabs(int nds) {
    if (!ImGui::BeginTabBar("shelf")) return;
    ImGuiTabItemFlags ds = nds ? 0 : ImGuiTabItemFlags_SetSelected;
    ImGuiTabItemFlags ns = nds ? ImGuiTabItemFlags_SetSelected : 0;
    if (ImGui::BeginTabItem("3DS", NULL, ds)) ImGui::EndTabItem();
    if (ImGui::IsItemClicked()) g_shelf = 0;
    if (ImGui::BeginTabItem("NDS/DSi", NULL, ns)) ImGui::EndTabItem();
    if (ImGui::IsItemClicked()) g_shelf = 1;
    ImGui::EndTabBar();
}

static void draw_picked(ImDrawList *dl, ImVec2 ip, float icon) {
    dl->AddRect(ImVec2(ip.x - 1.5f, ip.y - 1.5f), ImVec2(ip.x + icon + 1.5f, ip.y + icon + 1.5f),
                IM_COL32(115, 217, 140, 255), 3.f, 0, 3.f);
    float r = 7.f;
    ImVec2 c(ip.x + icon - 4.f, ip.y + 4.f);
    dl->AddCircleFilled(c, r, IM_COL32(12, 13, 15, 255), 16);
    dl->AddCircleFilled(c, r - 1.5f, IM_COL32(115, 217, 140, 255), 16);
    dl->AddLine(ImVec2(c.x - 3.2f, c.y + 0.2f), ImVec2(c.x - 1.0f, c.y + 2.6f), IM_COL32(12, 13, 15, 255), 1.7f);
    dl->AddLine(ImVec2(c.x - 1.0f, c.y + 2.6f), ImVec2(c.x + 3.4f, c.y - 2.4f), IM_COL32(12, 13, 15, 255), 1.7f);
}

static bool list_row(const char *id, bool active, bool marked, void *tex, const char *title, const char *sub, const char *side) {
    ImGui::PushID(id);
    ImVec2 origin = ImGui::GetCursorScreenPos();
    bool hit = ImGui::Selectable("##row", active, 0, ImVec2(0, 48));
    ImGui::SetCursorScreenPos(ImVec2(origin.x + 4, origin.y + 5));
    if (tex) {
        ImVec2 ip(origin.x + 4.f, origin.y + 5.f);
        ImGui::SetCursorScreenPos(ip);
        ImGui::Image((ImTextureID)tex, ImVec2(36, 36));
        if (marked) draw_picked(ImGui::GetWindowDrawList(), ip, 36.f);
        ImGui::SameLine();
    }
    ImGui::BeginGroup();
    char shown[96];
    clip_chars(shown, sizeof(shown), title, 14);
    if (marked) ImGui::TextColored(ImVec4(0.45f, 0.85f, 0.55f, 1.f), "%s", shown);
    else ImGui::TextUnformatted(shown);
    ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(0.60f, 0.64f, 0.68f, 1.f));
    ImGui::TextUnformatted(sub[0] ? sub : "-");
    ImGui::PopStyleColor();
    ImGui::EndGroup();
    if (side && side[0]) {
        float x = origin.x + ImGui::GetWindowWidth() - ImGui::CalcTextSize(side).x - 18.f;
        ImGui::SameLine();
        ImGui::SetCursorScreenPos(ImVec2(x, origin.y + 14));
        ImGui::TextUnformatted(side);
    }
    ImGui::SetCursorScreenPos(ImVec2(origin.x, origin.y + 48));
    ImGui::Dummy(ImVec2(0, 0));
    ImGui::PopID();
    return hit && !g_ate_click;
}

static bool g_follow;
static bool g_touch_down;
static bool g_touch_drag;
static float g_touch_x;
static float g_touch_y;
static float g_touch_last_y;

static void drag_scroll(void) {
    ImGuiIO &io = ImGui::GetIO();
    bool down = io.MouseDown[0] && io.MousePos.x >= 0.f && io.MousePos.y >= 0.f && io.MousePos.y < 240.f;
    if (!down) {
        g_ate_click = g_touch_down && g_touch_drag;
        g_touch_down = false;
        g_touch_drag = false;
        return;
    }
    bool hovered = ImGui::IsWindowHovered(ImGuiHoveredFlags_AllowWhenBlockedByActiveItem);
    if (!g_touch_down) {
        g_ate_click = false;
        if (!hovered) return;
        g_touch_down = true;
        g_touch_x = io.MousePos.x;
        g_touch_y = io.MousePos.y;
        g_touch_last_y = io.MousePos.y;
        return;
    }
    float moved_x = io.MousePos.x - g_touch_x;
    float moved_y = io.MousePos.y - g_touch_y;
    float delta = io.MousePos.y - g_touch_last_y;
    g_touch_last_y = io.MousePos.y;
    if (!g_touch_drag) {
        if (moved_x * moved_x + moved_y * moved_y < 64.f) return;
        g_touch_drag = true;
    }
    float next = ImGui::GetScrollY() - delta;
    if (next < 0.f) next = 0.f;
    float max_y = ImGui::GetScrollMaxY();
    if (next > max_y) next = max_y;
    ImGui::SetScrollY(next);
}

static float item_y(const TitleList *list, const int *visible, int cursor, int across, float row, bool grouped) {
    if (cursor < 0) return 0.f;
    if (across < 1) across = 1;
    if (!grouped) return (float)(cursor / across) * row;
    float y = 0.f;
    int last = -1;
    int col = 0;
    for (int i = 0; i <= cursor; i++) {
        int g = group_rank(&list->items[visible[i]]);
        if (g != last) {
            if (col) {
                y += row;
                col = 0;
            }
            y += GROUP_H;
            last = g;
        }
        if (i == cursor) return y;
        col++;
        if (col >= across) {
            y += row;
            col = 0;
        }
    }
    return y;
}

static void scroll_follow(const char *key, float y, float row) {
    float avail = ImGui::GetContentRegionAvail().y;
    if (avail < 1.f) avail = row;
    ImGuiID id = ImGui::GetID(key);
    static ImGuiID last = 0;
    static float top = 0.f;
    bool jump = last != id || g_follow;
    if (last != id) {
        top = 0.f;
        last = id;
    }
    if (!jump) return;
    if (y < top) top = y;
    if (y + row > top + avail) top = y + row - avail;
    if (top < 0.f) top = 0.f;
    g_follow = false;
    ImGui::SetNextWindowScroll(ImVec2(0.f, top));
}

static bool grid_cell(const char *id, bool active, bool marked, void *tex, const char *title, float cell) {
    (void)title;
    ImGui::PushID(id);
    ImGui::BeginGroup();
    ImVec2 origin = ImGui::GetCursorScreenPos();
    ImGui::PushStyleColor(ImGuiCol_Header, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderHovered, IM_COL32(0, 0, 0, 0));
    ImGui::PushStyleColor(ImGuiCol_HeaderActive, IM_COL32(0, 0, 0, 0));
    bool hit = ImGui::Selectable("##cell", active, 0, ImVec2(cell, GRID_STRIDE));
    ImGui::PopStyleColor(3);
    float icon = 44.f;
    ImVec2 ip(origin.x + (cell - icon) * 0.5f, origin.y + (GRID_STRIDE - icon) * 0.5f);
    ImDrawList *dl = ImGui::GetWindowDrawList();
    if (active) {
        dl->AddRectFilled(ImVec2(ip.x - 4.f, ip.y - 4.f), ImVec2(ip.x + icon + 4.f, ip.y + icon + 4.f),
                          IM_COL32(72, 124, 186, 255), 3.f);
    }
    if (tex) {
        ImGui::SetCursorScreenPos(ip);
        ImGui::Image((ImTextureID)tex, ImVec2(icon, icon));
    }
    if (marked) draw_picked(dl, ip, icon);
    ImGui::EndGroup();
    ImGui::PopID();
    return hit && !g_ate_click;
}

static void draw_top(const TitleList *list, const int *visible, int vcount, int cursor,
                     int nds, Result services, Result load_rc, const char *status, u32 tick) {
    ui_screen(GFX_TOP);
    panel("top");
    draw_clock_line();
    ImGui::Separator();
    ImVec2 block = ImGui::GetCursorScreenPos();
    storage_side(block.y);
    float wrap = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - 168.f;
    if (R_FAILED(services)) {
        ImGui::PushTextWrapPos(wrap);
        ImGui::Text("Could not start.");
        ImGui::TextWrapped("Install Cinder with FBI, then open its icon on the HOME Menu.");
        ImGui::PopTextWrapPos();
    } else if (R_FAILED(load_rc)) {
        ImGui::PushTextWrapPos(wrap);
        ImGui::Text("Could not list titles.");
        ImGui::TextWrapped("%s", explain_result(load_rc));
        ImGui::PopTextWrapPos();
    } else if (vcount <= 0 || cursor < 0 || cursor >= vcount) {
        ImGui::Text("%s", nds ? "No NDS/DSi software." : "No 3DS software.");
        if (!nds) ImGui::TextDisabled("Games, demos, updates, and DLC.");
    } else {
        const TitleRec *title = &list->items[visible[cursor]];
        titles_warm((TitleRec *)title);
        char ver[16];
        char size[16];
        titles_format_version(ver, sizeof(ver), title->version);
        titles_format_size(size, sizeof(size), title->size);
        void *tex = icon_tex(title, tick);
        if (tex) {
            ImGui::Image((ImTextureID)tex, ImVec2(48, 48));
            ImGui::SameLine();
        }
        ImGui::BeginGroup();
        char wrapped[192];
        char wrapped_pub[128];
        wrap_name(wrapped, sizeof(wrapped), title->name, 10);
        wrap_name(wrapped_pub, sizeof(wrapped_pub), title->publisher[0] ? title->publisher : "-", 8);
        ImGui::TextUnformatted(wrapped);
        ImGui::TextDisabled("%s", wrapped_pub);
        ImGui::Text("%s    v%s    %s", titles_kind_name(title->kind), ver,
                    title->licensed ? "Licensed" : "Homebrew");
        ImGui::Text("%s    %s", size, title->media == MEDIATYPE_SD ? "SD card" : "NAND");
        ImGui::TextDisabled("%s", title->product[0] ? title->product : "-");
        ImGui::TextDisabled("%016llX", (unsigned long long)title->id);
        ImGui::EndGroup();
    }
    if (status && status[0]) ImGui::TextWrapped("%s", status);
    static const int keys[] = {1, 6, 2, 3, 4, 5};
    const char *actions[] = {"select", g_grid ? "list" : "grid", "all", "install", "delete", "exit"};
    draw_prompts(keys, actions, 6);
    ImGui::End();
}

static const char *group_label(int pass) {
    if (pass == 1) return "Updates";
    if (pass == 2) return "DLC";
    if (pass == 3) return "Homebrew";
    return "Games";
}

static void group_head(const char *label) {
    ImVec2 p = ImGui::GetCursorScreenPos();
    ImGui::Dummy(ImVec2(1.f, GROUP_H));
    ImGui::SetCursorScreenPos(p);
    ImGui::TextDisabled("%s", label);
    ImGui::SetCursorScreenPos(ImVec2(p.x, p.y + GROUP_H));
}

static void draw_list(const TitleList *list, const int *visible, int vcount, int *cursor, int *nds, u32 tick) {
    ui_screen(GFX_BOTTOM);
    panel("list");
    shelf_tabs(*nds);
    if (corner_view()) g_grid = !g_grid;
    ImGui::TextDisabled("%d", vcount);
    u64 selected_bytes = 0;
    draw_selection(count_selected(list, &selected_bytes), selected_bytes, 0.f);
    char key[32];
    snprintf(key, sizeof(key), "%s-%s", *nds ? "titles-nds" : "titles-3ds", g_grid ? "g" : "l");
    bool grouped = !*nds;
    float row_h = g_grid ? GRID_STRIDE : 48.f;
    int across = g_grid ? GRID_COLS : 1;
    float y = item_y(list, visible, vcount > 0 ? *cursor : 0, across, row_h, grouped && vcount > 0);
    scroll_follow(key, y, row_h);
    if (g_grid) {
        float gap = 6.f;
        float cell = (ImGui::GetContentRegionAvail().x - gap * (float)(GRID_COLS - 1)) / (float)GRID_COLS;
        if (cell < 48.f) cell = 48.f;
        ImGui::BeginChild("grow", ImVec2(0, 0), false);
        drag_scroll();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 0.f));
        if (vcount <= 0) ImGui::Text("%s", *nds ? "No NDS/DSi software." : "No 3DS software.");
        int index = 0;
        for (int pass = 0; pass < (grouped ? 4 : 1); pass++) {
            int start = index;
            if (grouped) {
                while (index < vcount && group_rank(&list->items[visible[index]]) == pass) index++;
            } else {
                index = vcount;
            }
            int n = index - start;
            if (n <= 0) continue;
            if (grouped) group_head(group_label(pass));
            int grows = (n + GRID_COLS - 1) / GRID_COLS;
            ImGuiListClipper clipper;
            clipper.Begin(grows, GRID_STRIDE);
            while (clipper.Step()) {
                for (int gr = clipper.DisplayStart; gr < clipper.DisplayEnd; gr++) {
                    for (int col = 0; col < GRID_COLS; col++) {
                        int local = gr * GRID_COLS + col;
                        if (local >= n) break;
                        int row = start + local;
                        if (col > 0) ImGui::SameLine(0.f, gap);
                        const TitleRec *title = &list->items[visible[row]];
                        char id[16];
                        snprintf(id, sizeof(id), "g%d", row);
                        if (grid_cell(id, row == *cursor, title->selected, icon_tex(title, tick), title->name, cell)) {
                            *cursor = row;
                            if (title->deletable) {
                                TitleRec *mut = (TitleRec *)&list->items[visible[row]];
                                mut->selected = !mut->selected;
                            }
                        }
                    }
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
    } else {
        ImGui::BeginChild("rows", ImVec2(0, 0), false);
        drag_scroll();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 0.f));
        if (vcount <= 0) ImGui::Text("%s", *nds ? "No NDS/DSi software." : "No 3DS software.");
        int index = 0;
        for (int pass = 0; pass < (grouped ? 4 : 1); pass++) {
            int start = index;
            if (grouped) {
                while (index < vcount && group_rank(&list->items[visible[index]]) == pass) index++;
            } else {
                index = vcount;
            }
            int n = index - start;
            if (n <= 0) continue;
            if (grouped) group_head(group_label(pass));
            ImGuiListClipper clipper;
            clipper.Begin(n, 48.f);
            while (clipper.Step()) {
                for (int local = clipper.DisplayStart; local < clipper.DisplayEnd; local++) {
                    int row = start + local;
                    const TitleRec *title = &list->items[visible[row]];
                    char size[16];
                    char sub[96];
                    titles_format_size(size, sizeof(size), title->size);
                    clip_chars(sub, sizeof(sub), title->publisher[0] ? title->publisher : "-", 14);
                    char id[16];
                    snprintf(id, sizeof(id), "t%d", row);
                    if (list_row(id, row == *cursor, title->selected, icon_tex(title, tick), title->name, sub, size)) {
                        *cursor = row;
                        if (title->deletable) {
                            TitleRec *mut = (TitleRec *)&list->items[visible[row]];
                            mut->selected = !mut->selected;
                        }
                    }
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    ImGui::End();
}

static void draw_confirm(u32 count, u64 bytes, bool *related, bool *tickets, int *action) {
    ui_screen(GFX_BOTTOM);
    panel("confirm");
    char size[16];
    titles_format_size(size, sizeof(size), bytes);
    ImGui::Text("Delete %lu titles?", (unsigned long)count);
    ImGui::Text("Size %s", size);
    ImGui::Checkbox("Updates and DLC", related);
    ImGui::Checkbox("Delete tickets", tickets);
    ImGui::TextWrapped("Tickets stay off unless you need the title gone for good. Saves may remain either way.");
    if (ImGui::Button("Delete", ImVec2(120, 28))) *action = 1;
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 28))) *action = 2;
    ImGui::End();
}

static void draw_result(const char *status, int *action) {
    ui_screen(GFX_TOP);
    panel("result-top");
    draw_clock_line();
    ImGui::Separator();
    ImGui::Dummy(ImVec2(0.f, 36.f));
    ImGui::TextWrapped("%s", status && status[0] ? status : "Done.");
    ImGui::End();
    ui_screen(GFX_BOTTOM);
    panel("result");
    ImGui::Dummy(ImVec2(0.f, 48.f));
    ImGui::TextWrapped("%s", status && status[0] ? status : "Done.");
    if (ImGui::Button("Back", ImVec2(120, 28))) *action = 2;
    ImGui::End();
}

static void on_progress(u32 index, u32 total, u64 id, Result rc, bool ticket_ok, void *user) {
    bool tickets = *(bool *)user;
    ui_screen(GFX_BOTTOM);
    panel("progress");
    ImGui::Text("Deleting %lu / %lu", (unsigned long)(index + 1), (unsigned long)total);
    ImGui::TextDisabled("%016llX", (unsigned long long)id);
    if (R_FAILED(rc)) ImGui::TextWrapped("%s", explain_result(rc));
    else ImGui::Text("Removed");
    if (tickets && R_SUCCEEDED(rc) && !ticket_ok) ImGui::TextDisabled("Ticket kept");
    float frac = total ? (float)(index + 1) / (float)total : 1.f;
    ImGui::ProgressBar(frac, ImVec2(-1, 18));
    ImGui::End();
    ui_present();
}

static Result delete_selected(TitleList *list, bool related, bool tickets) {
    u32 count = titles_collect(list, g_ids, TITLE_CAP, related);
    if (count == 0) return 0;

    u32 sd_n = 0;
    for (u32 i = 0; i < count; i++) {
        if (media_of(list, g_ids[i]) != MEDIATYPE_SD) continue;
        u64 swap = g_ids[sd_n];
        g_ids[sd_n] = g_ids[i];
        g_ids[i] = swap;
        sd_n++;
    }

    Result rc = 0;
    if (sd_n > 0) {
        Result one = titles_delete(MEDIATYPE_SD, g_ids, sd_n, tickets, on_progress, &tickets);
        if (R_FAILED(one)) rc = one;
    }
    if (count > sd_n) {
        Result one = titles_delete(MEDIATYPE_NAND, g_ids + sd_n, count - sd_n, tickets, on_progress, &tickets);
        if (R_FAILED(one)) rc = one;
    }
    return rc;
}

static bool in_cias_dir(const CiaRec *cia) {
    return cia->path[1] == 'c' && cia->path[2] == 'i' && cia->path[3] == 'a' &&
           cia->path[4] == 's' && cia->path[5] == '/';
}

static u32 count_cias(const CiaList *list, u64 *bytes) {
    u32 count = 0;
    *bytes = 0;
    if (!list->items) return 0;
    for (u32 i = 0; i < list->count; i++) {
        if (list->items[i].selected && list->items[i].installable) {
            count++;
            *bytes += list->items[i].file_size;
        }
    }
    return count;
}

static void toggle_cias(CiaList *list) {
    if (!list->items) return;
    int selectable = 0;
    int selected = 0;
    for (u32 i = 0; i < list->count; i++) {
        if (!list->items[i].installable) continue;
        selectable++;
        if (list->items[i].selected) selected++;
    }
    if (selectable == 0) return;
    bool on = selected < selectable;
    for (u32 i = 0; i < list->count; i++) {
        if (list->items[i].installable) list->items[i].selected = on;
    }
}

static const char *cia_label(const CiaRec *cia) {
    return cia->title_name[0] ? cia->title_name : cia->name;
}

static void draw_install_top(const CiaList *cias, int cursor, const char *status) {
    ui_screen(GFX_TOP);
    panel("itop");
    draw_clock_line();
    ImGui::Separator();
    ImVec2 block = ImGui::GetCursorScreenPos();
    storage_side(block.y);
    float wrap = ImGui::GetWindowPos().x + ImGui::GetWindowContentRegionMax().x - 168.f;
    int n = cias->items ? (int)cias->count : 0;
    if (R_FAILED(cias->rc)) {
        ImGui::PushTextWrapPos(wrap);
        ImGui::Text("Could not read the SD card.");
        ImGui::TextWrapped("%s", explain_result(cias->rc));
        ImGui::PopTextWrapPos();
    } else if (n <= 0 || cursor < 0 || cursor >= n) {
        ImGui::Text("No CIAs found.");
        ImGui::PushTextWrapPos(wrap);
        ImGui::TextDisabled("place cias in sdmc:/cias/ or sdmc:/");
        ImGui::PopTextWrapPos();
    } else {
        const CiaRec *cia = &cias->items[cursor];
        void *tex = cia->icon ? ui_image(cia->icon, cia->icon_w > 0 ? cia->icon_w : 48, false) : NULL;
        if (tex) {
            ImGui::Image((ImTextureID)tex, ImVec2(48, 48));
            ImGui::SameLine();
        }
        ImGui::BeginGroup();
        char wrapped[192];
        char wrapped_pub[128];
        wrap_name(wrapped, sizeof(wrapped), cia_label(cia), 10);
        wrap_name(wrapped_pub, sizeof(wrapped_pub), cia->publisher[0] ? cia->publisher : "-", 8);
        ImGui::TextUnformatted(wrapped);
        ImGui::TextDisabled("%s", wrapped_pub);
        ImGui::TextDisabled("%s", in_cias_dir(cia) ? "sdmc:/cias/" : "sdmc:/");
        ImGui::EndGroup();
        char ver[16];
        char file_s[16];
        char inst_s[16];
        titles_format_version(ver, sizeof(ver), cia->version);
        titles_format_size(file_s, sizeof(file_s), cia->file_size);
        titles_format_size(inst_s, sizeof(inst_s), cia->install_size);
        if (!cia->header_ok) {
            ImGui::Text("Not a readable CIA.");
            ImGui::TextDisabled("File %s", file_s);
        } else {
            if (!cia->complete) ImGui::TextColored(ImVec4(0.90f, 0.77f, 0.47f, 1.f), "Incomplete. Left alone.");
            ImGui::Text("v%s    %s    %s", ver, titles_kind_name(cia->kind),
                        cia->licensed ? "Licensed" : "Homebrew");
            ImGui::Text("File %s", file_s);
            if (cia->install_size) ImGui::Text("Install %s", inst_s);
            ImGui::TextDisabled("%s", cia->media == MEDIATYPE_SD ? "SD card" : "NAND");
            if (cia->title_id) ImGui::SameLine();
            if (cia->title_id) ImGui::TextDisabled("%016llX", (unsigned long long)cia->title_id);
        }
    }
    if (status && status[0]) ImGui::TextWrapped("%s", status);
    static const int keys[] = {1, 6, 2, 3, 4, 5};
    const char *actions[] = {"select", g_grid ? "list" : "grid", "all", "titles", "install", "exit"};
    draw_prompts(keys, actions, 6);
    ImGui::End();
}

static void draw_install_list(CiaList *cias, int *cursor, char *status, size_t status_n) {
    ui_screen(GFX_BOTTOM);
    panel("cias");
    int n = cias->items ? (int)cias->count : 0;
    ImGui::Text("CIAs");
    ImGui::SameLine();
    ImGui::TextDisabled("%d", n);
    if (corner_view()) g_grid = !g_grid;
    u64 selected_bytes = 0;
    draw_selection(count_cias(cias, &selected_bytes), selected_bytes, 36.f);
    if (g_grid) {
        float gap = 6.f;
        float cell = (ImGui::GetContentRegionAvail().x - gap * (float)(GRID_COLS - 1)) / (float)GRID_COLS;
        if (cell < 48.f) cell = 48.f;
        scroll_follow("cias-g", (float)(*cursor / GRID_COLS) * GRID_STRIDE, GRID_STRIDE);
        ImGui::BeginChild("cgrow", ImVec2(0, 0), false);
        drag_scroll();
        if (n <= 0) ImGui::Text("No CIAs found.");
        int grows = (n + GRID_COLS - 1) / GRID_COLS;
        ImGuiListClipper clipper;
        clipper.Begin(grows, GRID_STRIDE);
        while (clipper.Step()) {
            for (int gr = clipper.DisplayStart; gr < clipper.DisplayEnd; gr++) {
                for (int col = 0; col < GRID_COLS; col++) {
                    int row = gr * GRID_COLS + col;
                    if (row >= n) break;
                    if (col > 0) ImGui::SameLine(0.f, gap);
                    CiaRec *cia = &cias->items[row];
                    char id[16];
                    snprintf(id, sizeof(id), "cg%d", row);
                    void *tex = cia->icon ? ui_image(cia->icon, cia->icon_w > 0 ? cia->icon_w : 48, false) : NULL;
                    if (grid_cell(id, row == *cursor, cia->selected, tex, cia_label(cia), cell)) {
                        *cursor = row;
                        if (cia->installable) cia->selected = !cia->selected;
                        else if (status && status_n) {
                            if (cia->complete && cia->kind == KIND_SYSTEM)
                                snprintf(status, status_n, "System software is left alone.");
                            else snprintf(status, status_n, "Incomplete CIA.");
                        }
                    }
                }
            }
        }
        ImGui::EndChild();
    } else {
        scroll_follow("cias-l", (float)*cursor * 48.f, 48.f);
        ImGui::BeginChild("crows", ImVec2(0, 0), false);
        drag_scroll();
        ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(8.f, 0.f));
        if (n <= 0) ImGui::Text("No CIAs found.");
        ImGuiListClipper clipper;
        clipper.Begin(n, 48.f);
        while (clipper.Step()) {
            for (int row = clipper.DisplayStart; row < clipper.DisplayEnd; row++) {
                CiaRec *cia = &cias->items[row];
                char size[16];
                char sub[96];
                titles_format_size(size, sizeof(size), cia->file_size);
                if (!cia->header_ok) snprintf(sub, sizeof(sub), "bad file");
                else if (!cia->complete) snprintf(sub, sizeof(sub), "incomplete");
                else clip_chars(sub, sizeof(sub), cia->publisher[0] ? cia->publisher : "-", 14);
                char id[16];
                snprintf(id, sizeof(id), "c%d", row);
                void *tex = cia->icon ? ui_image(cia->icon, cia->icon_w > 0 ? cia->icon_w : 48, false) : NULL;
                if (list_row(id, row == *cursor, cia->selected, tex, cia_label(cia), sub, size)) {
                    *cursor = row;
                    if (cia->installable) cia->selected = !cia->selected;
                    else if (status && status_n) {
                        if (cia->complete && cia->kind == KIND_SYSTEM)
                            snprintf(status, status_n, "System software is left alone.");
                        else snprintf(status, status_n, "Incomplete CIA.");
                    }
                }
            }
        }
        ImGui::PopStyleVar();
        ImGui::EndChild();
    }
    ImGui::End();
}

static void draw_install_confirm(u32 count, u64 bytes, int *action) {
    ui_screen(GFX_BOTTOM);
    panel("iconfirm");
    char size[16];
    titles_format_size(size, sizeof(size), bytes);
    if (count == 1) ImGui::Text("Install 1 CIA?");
    else ImGui::Text("Install %lu CIAs?", (unsigned long)count);
    ImGui::Text("%s", size);
    if (ImGui::Button("Install", ImVec2(120, 28))) *action = 1;
    ImGui::SameLine();
    if (ImGui::Button("Cancel", ImVec2(120, 28))) *action = 2;
    ImGui::End();
}

static void draw_reinstall(const CiaList *cias, int *action) {
    int n = 0;
    const char *name = "";
    if (cias->items) {
        for (u32 i = 0; i < cias->count && i < CIA_CAP; i++) {
            if (!g_replace[i]) continue;
            n++;
            name = cia_label(&cias->items[i]);
        }
    }
    ui_screen(GFX_BOTTOM);
    panel("reinstall");
    if (n <= 1) {
        ImGui::TextWrapped("%s is already installed.", name[0] ? name : "That title");
        ImGui::Text("Reinstall it?");
    } else {
        ImGui::Text("%d titles are already installed.", n);
        ImGui::Text("Reinstall them?");
    }
    if (ImGui::Button("Reinstall", ImVec2(120, 28))) *action = 1;
    ImGui::SameLine();
    if (ImGui::Button("No", ImVec2(120, 28))) *action = 2;
    ImGui::End();
}

static bool g_quit;
static bool g_pending_reload;

static bool on_cia_pump(const CiaRec *cia, u32 index, u32 total, u64 wrote, void *user) {
    (void)user;
    if (!aptMainLoop()) {
        g_quit = true;
        return false;
    }
    hidScanInput();
    u32 held = hidKeysHeld();
    if ((held & KEY_START) && (held & KEY_SELECT)) {
        g_quit = true;
        return false;
    }
    if (hidKeysDown() & KEY_B) return false;

    static u64 last_draw = ~(u64)0;
    static u32 last_index = 0xFFFFFFFFu;
    if (index != last_index) {
        last_draw = ~(u64)0;
        last_index = index;
    }
    if (last_draw != ~(u64)0 && wrote != cia->file_size && wrote > last_draw &&
        wrote - last_draw < 256ull * 1024ull) {
        return true;
    }
    last_draw = wrote;

    ui_screen(GFX_TOP);
    panel("ipump");
    ImGui::Text("Installing %lu / %lu", (unsigned long)(index + 1), (unsigned long)total);
    ImGui::TextWrapped("%s", cia->name);
    char done[16];
    char all[16];
    titles_format_size(done, sizeof(done), wrote);
    titles_format_size(all, sizeof(all), cia->file_size);
    ImGui::Text("%s / %s", done, all);
    ImGui::TextDisabled("B cancel");
    ImGui::End();

    ui_screen(GFX_BOTTOM);
    panel("ibar");
    float frac = cia->file_size ? (float)((double)wrote / (double)cia->file_size) : 0.f;
    if (frac < 0) frac = 0;
    if (frac > 1) frac = 1;
    ImGui::ProgressBar(frac, ImVec2(-1, 22));
    ImGui::End();
    ui_present();
    return true;
}

static void install_selected(CiaList *cias, TitleList *titles, Result *load_rc, char *status, size_t status_n, bool replace, int *blocked, int *focus) {
    (void)titles;
    (void)load_rc;
    if (blocked) *blocked = 0;
    u32 total = 0;
    if (cias->items) {
        for (u32 i = 0; i < cias->count && i < CIA_CAP; i++) {
            CiaRec *cia = &cias->items[i];
            if (replace) {
                if (!g_replace[i]) continue;
            } else if (!cia->selected || !cia->installable) {
                continue;
            }
            total++;
        }
    }
    if (total == 0) {
        snprintf(status, status_n, "Nothing to install.");
        return;
    }

    u32 index = 0;
    u32 ok = 0;
    int ask = 0;
    bool home = false;
    Result rc = 0;
    bool cancelled = false;
    for (u32 i = 0; i < cias->count && i < CIA_CAP; i++) {
        CiaRec *cia = &cias->items[i];
        if (replace) {
            if (!g_replace[i]) continue;
        } else if (!cia->selected || !cia->installable) {
            continue;
        }
        if (!replace) g_replace[i] = 0;
        Result one = cias_install_one(cia, index, total, on_cia_pump, NULL, replace);
        if (g_quit || one == CIA_ERR_CANCEL) {
            cancelled = true;
            rc = one;
            break;
        }
        if (R_FAILED(one)) {
            if (!replace && already_installed(one)) {
                g_replace[i] = 1;
                ask++;
                if (blocked) *blocked = ask;
            } else {
                rc = one;
                g_replace[i] = 0;
            }
        } else {
            ok++;
            if (titles_is_home_menu(cia->title_id)) home = true;
            cia->selected = false;
            g_replace[i] = 0;
        }
        index++;
    }

    if (!g_quit && ok > 0) g_pending_reload = true;
    if (ask > 0 && !cancelled && !g_quit) {
        if (focus) {
            for (u32 i = 0; i < cias->count && i < CIA_CAP; i++) {
                if (g_replace[i]) {
                    *focus = (int)i;
                    break;
                }
            }
        }
        if (R_FAILED(rc)) snprintf(status, status_n, "Installed %lu. %s", (unsigned long)ok, explain_result(rc));
        else if (ok) snprintf(status, status_n, home ? "Installed %lu. Reboot to use the new HOME Menu." : "Installed %lu.", (unsigned long)ok);
        else status[0] = '\0';
        return;
    }
    if (g_quit) snprintf(status, status_n, "Stopped.");
    else if (cancelled) snprintf(status, status_n, "Cancelled. Installed %lu.", (unsigned long)ok);
    else if (R_FAILED(rc)) snprintf(status, status_n, "Installed %lu. %s", (unsigned long)ok, explain_result(rc));
    else if (home) snprintf(status, status_n, "Installed %lu. Reboot to use the new HOME Menu.", (unsigned long)ok);
    else snprintf(status, status_n, "Installed %lu.", (unsigned long)ok);
}

static void enter_install(CiaList *cias, const TitleList *titles, int *cursor, int *scroll) {
    ui_screen(GFX_BOTTOM);
    panel("reading");
    ImGui::Text("Reading CIAs...");
    ImGui::End();
    ui_present();
    ui_drop_images();
    cias_load(cias);
    cias_link(cias, titles);
    memset(g_replace, 0, sizeof(g_replace));
    *cursor = 0;
    *scroll = 0;
}

int main(int argc, char **argv) {
    (void)argc;
    (void)argv;

    ui_init();
    titles_set_load_pump(load_pump);
    bgm_start();

    Result am_rc = amInit();
    Result fs_rc = fsInit();
    g_ptmu = R_SUCCEEDED(ptmuInit());
    Result services = R_FAILED(am_rc) ? am_rc : fs_rc;

    TitleList list;
    memset(&list, 0, sizeof(list));
    CiaList cias;
    memset(&cias, 0, sizeof(cias));
    Result load_rc = 0;
    if (R_SUCCEEDED(services)) reload(&list, &load_rc, true);

    Mode mode = MODE_LIST;
    int nds = 0;
    int cursor = 0;
    int scroll = 0;
    int icursor = 0;
    int iscroll = 0;
    int down_timer = 0;
    int up_timer = 0;
    int left_timer = 0;
    int right_timer = 0;
    bool include_related = true;
    bool delete_tickets = false;
    char status[192];
    status[0] = '\0';

    u32 tick = 0;

    while (aptMainLoop()) {
        bgm_poll();
        hidScanInput();
        u32 down = hidKeysDown();
        u32 held = hidKeysHeld();
        int vcount = build_visible(&list, nds, g_visible);
        if (cursor >= vcount) cursor = vcount > 0 ? vcount - 1 : 0;
        if (cursor < scroll) scroll = cursor;
        if (cursor >= scroll + LIST_ROWS) scroll = cursor - LIST_ROWS + 1;
        if (scroll < 0) scroll = 0;

        if ((held & KEY_START) && (held & KEY_SELECT)) break;

        if (mode == MODE_RESULT) {
            if (g_quit) break;
            if (down & (KEY_A | KEY_B | KEY_START)) {
                status[0] = '\0';
                mode = MODE_LIST;
                if (g_pending_reload) {
                    reload(&list, &load_rc, true);
                    g_pending_reload = false;
                    cursor = 0;
                    scroll = 0;
                }
            }
        } else if (mode == MODE_CONFIRM) {
            if (down & KEY_B) mode = MODE_LIST;
            if (down & KEY_Y) include_related = !include_related;
            if (down & KEY_X) delete_tickets = !delete_tickets;
            if (down & KEY_A) {
                u64 bytes = 0;
                u32 marked = count_selected(&list, &bytes);
                if (marked == 0) {
                    snprintf(status, sizeof(status), "Nothing selected.");
                    mode = MODE_LIST;
                } else {
                    bool tickets = delete_tickets;
                    Result rc = delete_selected(&list, include_related, tickets);
                    g_pending_reload = true;
                    cursor = 0;
                    scroll = 0;
                    if (R_FAILED(rc)) snprintf(status, sizeof(status), "Could not remove them. %s", explain_result(rc));
                    else snprintf(status, sizeof(status), "Removed %lu titles.", (unsigned long)marked);
                    mode = MODE_RESULT;
                }
            }
        } else if (mode == MODE_INSTALL_CONFIRM) {
            if (down & KEY_B) mode = MODE_INSTALL;
            if (down & KEY_A) {
                int blocked = 0;
                install_selected(&cias, &list, &load_rc, status, sizeof(status), false, &blocked, &icursor);
                if (g_quit) break;
                mode = blocked > 0 ? MODE_REINSTALL : MODE_RESULT;
            }
        } else if (mode == MODE_REINSTALL) {
            if (down & KEY_B) {
                memset(g_replace, 0, sizeof(g_replace));
                status[0] = '\0';
                mode = MODE_INSTALL;
                if (g_pending_reload) {
                    reload(&list, &load_rc, true);
                    g_pending_reload = false;
                }
            } else if (down & KEY_A) {
                install_selected(&cias, &list, &load_rc, status, sizeof(status), true, NULL, NULL);
                if (g_quit) break;
                mode = MODE_RESULT;
            }
        } else if (mode == MODE_INSTALL && R_SUCCEEDED(services)) {
            int n = cias.items ? (int)cias.count : 0;
            if (icursor >= n) icursor = n > 0 ? n - 1 : 0;
            if (icursor < iscroll) iscroll = icursor;
            if (icursor >= iscroll + CIA_ROWS) iscroll = icursor - CIA_ROWS + 1;
            if (iscroll < 0) iscroll = 0;
            if (down & KEY_B) {
                mode = MODE_LIST;
            } else {
                int dx = 0;
                int dy = 0;
                if (repeat_key(down, held, KEY_DDOWN, &down_timer) || (down & KEY_CPAD_DOWN)) dy = 1;
                else if (repeat_key(down, held, KEY_DUP, &up_timer) || (down & KEY_CPAD_UP)) dy = -1;
                if (g_grid) {
                    if (repeat_key(down, held, KEY_DRIGHT, &right_timer) || (down & KEY_CPAD_RIGHT)) dx = 1;
                    else if (repeat_key(down, held, KEY_DLEFT, &left_timer) || (down & KEY_CPAD_LEFT)) dx = -1;
                }
                if (dx || dy) {
                    nudge(&icursor, n, dx, dy);
                    g_follow = true;
                }
                if (down & KEY_Y) g_grid = !g_grid;
                if (down & KEY_A && n > 0) {
                    CiaRec *cia = &cias.items[icursor];
                    if (cia->installable) cia->selected = !cia->selected;
                    else snprintf(status, sizeof(status), "Incomplete CIA.");
                }
                if (down & KEY_X) toggle_cias(&cias);
                if (down & KEY_SELECT) {
                    status[0] = '\0';
                    mode = MODE_LIST;
                }
                if (down & KEY_START) {
                    u64 bytes = 0;
                    if (count_cias(&cias, &bytes) > 0) {
                        status[0] = '\0';
                        mode = MODE_INSTALL_CONFIRM;
                    } else snprintf(status, sizeof(status), "Select a Cia.");
                }
            }
        } else if (mode == MODE_LIST && R_SUCCEEDED(services)) {
            int dx = 0;
            int dy = 0;
            if (repeat_key(down, held, KEY_DDOWN, &down_timer) || (down & KEY_CPAD_DOWN)) dy = 1;
            else if (repeat_key(down, held, KEY_DUP, &up_timer) || (down & KEY_CPAD_UP)) dy = -1;
            if (g_grid) {
                if (repeat_key(down, held, KEY_DRIGHT, &right_timer) || (down & KEY_CPAD_RIGHT)) dx = 1;
                else if (repeat_key(down, held, KEY_DLEFT, &left_timer) || (down & KEY_CPAD_LEFT)) dx = -1;
            }
            if (dx || dy) {
                if (!nds && g_grid) nudge_grouped(&list, g_visible, vcount, &cursor, dx, dy);
                else nudge(&cursor, vcount, dx, dy);
                g_follow = true;
            }
            if (down & KEY_Y) g_grid = !g_grid;
            if (down & KEY_A && vcount > 0) {
                TitleRec *title = &list.items[g_visible[cursor]];
                if (title->deletable) title->selected = !title->selected;
            }
            if (down & KEY_X) toggle_visible(&list, g_visible, vcount);
            if (down & (KEY_L | KEY_R)) {
                nds = !nds;
                cursor = 0;
                scroll = 0;
                status[0] = '\0';
            }
            if (down & KEY_SELECT) {
                enter_install(&cias, &list, &icursor, &iscroll);
                status[0] = '\0';
                mode = MODE_INSTALL;
            }
            if (down & KEY_START) {
                u64 bytes = 0;
                if (count_selected(&list, &bytes) > 0) {
                    status[0] = '\0';
                    mode = MODE_CONFIRM;
                } else snprintf(status, sizeof(status), "Select a title first.");
            }
        }

        if (g_shelf == 0 || g_shelf == 1) {
            if (nds != g_shelf) {
                nds = g_shelf;
                cursor = 0;
                scroll = 0;
                status[0] = '\0';
            }
            g_shelf = -1;
        }

        vcount = build_visible(&list, nds, g_visible);
        if (cursor >= vcount) cursor = vcount > 0 ? vcount - 1 : 0;

        int action = 0;
        if (mode == MODE_RESULT) {
            draw_result(status, &action);
        } else if (mode == MODE_INSTALL || mode == MODE_INSTALL_CONFIRM || mode == MODE_REINSTALL) {
            draw_install_top(&cias, icursor, status);
        } else {
            draw_top(&list, g_visible, vcount, cursor, nds, services, load_rc, status, tick);
        }

        if (mode == MODE_CONFIRM) {
            u32 count = titles_collect(&list, g_ids, TITLE_CAP, include_related);
            u64 bytes = 0;
            for (u32 i = 0; i < count; i++) {
                for (u32 j = 0; j < list.count; j++) {
                    if (list.items[j].id == g_ids[i]) bytes += list.items[j].size;
                }
            }
            draw_confirm(count, bytes, &include_related, &delete_tickets, &action);
        } else if (mode == MODE_INSTALL_CONFIRM) {
            u64 bytes = 0;
            draw_install_confirm(count_cias(&cias, &bytes), bytes, &action);
        } else if (mode == MODE_REINSTALL) {
            draw_reinstall(&cias, &action);
        } else if (mode == MODE_INSTALL) {
            int n = cias.items ? (int)cias.count : 0;
            if (icursor >= n) icursor = n > 0 ? n - 1 : 0;
            draw_install_list(&cias, &icursor, status, sizeof(status));
        } else if (mode != MODE_RESULT) {
            draw_list(&list, g_visible, vcount, &cursor, &nds, tick);
        }

        ui_present();
        if (action == 2 && mode == MODE_CONFIRM) mode = MODE_LIST;
        if (action == 2 && mode == MODE_INSTALL_CONFIRM) mode = MODE_INSTALL;
        if (action == 2 && mode == MODE_REINSTALL) {
            memset(g_replace, 0, sizeof(g_replace));
            status[0] = '\0';
            mode = MODE_INSTALL;
            if (g_pending_reload) {
                reload(&list, &load_rc, true);
                g_pending_reload = false;
            }
        }
        if (action == 2 && mode == MODE_RESULT) {
            status[0] = '\0';
            mode = MODE_LIST;
            if (g_pending_reload) {
                reload(&list, &load_rc, true);
                g_pending_reload = false;
                cursor = 0;
                scroll = 0;
            }
        }
        if (action == 1 && mode == MODE_CONFIRM) {
            u64 bytes = 0;
            u32 marked = count_selected(&list, &bytes);
            if (marked == 0) {
                snprintf(status, sizeof(status), "Nothing selected.");
                mode = MODE_LIST;
            } else {
                bool tickets = delete_tickets;
                Result rc = delete_selected(&list, include_related, tickets);
                g_pending_reload = true;
                cursor = 0;
                scroll = 0;
                if (R_FAILED(rc)) snprintf(status, sizeof(status), "Could not remove them. %s", explain_result(rc));
                else snprintf(status, sizeof(status), "Removed %lu titles.", (unsigned long)marked);
                mode = MODE_RESULT;
            }
        }
        if (action == 1 && mode == MODE_INSTALL_CONFIRM) {
            int blocked = 0;
            install_selected(&cias, &list, &load_rc, status, sizeof(status), false, &blocked, &icursor);
            if (g_quit) break;
            mode = blocked > 0 ? MODE_REINSTALL : MODE_RESULT;
        }
        if (action == 1 && mode == MODE_REINSTALL) {
            install_selected(&cias, &list, &load_rc, status, sizeof(status), true, NULL, NULL);
            if (g_quit) break;
            mode = MODE_RESULT;
        }
        tick++;
    }

    ui_drop_images();
    titles_free(&list);
    cias_free(&cias);
    bgm_stop();
    if (R_SUCCEEDED(fs_rc)) fsExit();
    if (R_SUCCEEDED(am_rc)) amExit();
    if (g_ptmu) ptmuExit();
    ui_exit();
    return 0;
}
