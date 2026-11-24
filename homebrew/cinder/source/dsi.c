#include "titles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TWL_ARCHIVE ((FS_ArchiveID)ARCHIVE_NAND_TWL_FS)
#define BANNER_MAX 0x23C0

static FS_Archive g_archive;
static bool g_archive_open;

void dsi_shutdown(void) {
    if (g_archive_open) {
        FSUSER_CloseArchive(g_archive);
        g_archive_open = false;
    }
}

static bool archive_open(void) {
    if (g_archive_open) return true;
    FS_Path empty;
    empty.type = PATH_EMPTY;
    empty.size = 1;
    empty.data = "";
    if (R_FAILED(FSUSER_OpenArchive(&g_archive, TWL_ARCHIVE, empty))) return false;
    g_archive_open = true;
    return true;
}

static Result open_file(const char *path, Handle *out) {
    Result rc = FSUSER_OpenFile(out, g_archive, fsMakePath(PATH_ASCII, path), FS_OPEN_READ, 0);
    if (R_SUCCEEDED(rc)) return rc;
    u16 wide[96];
    size_t n = 0;
    for (; path[n] && n < 95; n++) wide[n] = (u16)(unsigned char)path[n];
    wide[n] = 0;
    FS_Path p;
    p.type = PATH_UTF16;
    p.size = (u32)((n + 1) * 2);
    p.data = wide;
    return FSUSER_OpenFile(out, g_archive, p, FS_OPEN_READ, 0);
}

static Result open_dir(const char *path, Handle *out) {
    Result rc = FSUSER_OpenDirectory(out, g_archive, fsMakePath(PATH_ASCII, path));
    if (R_SUCCEEDED(rc)) return rc;
    u16 wide[96];
    size_t n = 0;
    for (; path[n] && n < 95; n++) wide[n] = (u16)(unsigned char)path[n];
    wide[n] = 0;
    FS_Path p;
    p.type = PATH_UTF16;
    p.size = (u32)((n + 1) * 2);
    p.data = wide;
    return FSUSER_OpenDirectory(out, g_archive, p);
}

static u16 rgb555_to_565(u16 c) {
    u16 r = c & 0x1F;
    u16 g = (c >> 5) & 0x1F;
    u16 b = (c >> 10) & 0x1F;
    u16 g6 = (u16)((g << 1) | (g >> 4));
    return (u16)((r << 11) | (g6 << 5) | b);
}

static void decode_nds_icon(u16 *dst, const u8 *bitmap, const u16 *palette, bool flip_h, bool flip_v) {
    for (int ty = 0; ty < 4; ty++) {
        for (int tx = 0; tx < 4; tx++) {
            const u8 *tile = bitmap + ((ty * 4 + tx) * 32);
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    u8 byte = tile[y * 4 + (x >> 1)];
                    u8 idx = (x & 1) ? (u8)(byte >> 4) : (u8)(byte & 0x0F);
                    int dx = tx * 8 + x;
                    int dy = ty * 8 + y;
                    if (flip_h) dx = 31 - dx;
                    if (flip_v) dy = 31 - dy;
                    dst[dy * 32 + dx] = idx == 0 ? 0 : rgb555_to_565(palette[idx]);
                }
            }
        }
    }
}

u16 titles_anim_token(const IconAnim *anim, u32 tick) {
    if (!anim || anim->seq_len == 0) return 0;
    u32 total = 0;
    for (u8 i = 0; i < anim->seq_len; i++) {
        u32 d = anim->seq[i] & 0xFF;
        if (d == 0) continue;
        total += d;
    }
    if (total == 0) return 0;
    u32 t = tick % total;
    for (u8 i = 0; i < anim->seq_len; i++) {
        u32 d = anim->seq[i] & 0xFF;
        if (d == 0) continue;
        if (t < d) return anim->seq[i];
        t -= d;
    }
    return 0;
}

bool titles_anim_frame(const IconAnim *anim, u32 tick, u16 *dst) {
    u16 token = titles_anim_token(anim, tick);
    if (!token || !dst) return false;
    u8 pal = (u8)((token >> 11) & 7);
    u8 bmp = (u8)((token >> 8) & 7);
    decode_nds_icon(dst, anim->bitmap[bmp], anim->palette[pal],
                    (token & 0x4000) != 0, (token & 0x8000) != 0);
    return true;
}

static void banner_to_utf8(char *out, size_t out_len, const u16 *in, size_t units) {
    size_t o = 0;
    if (out_len == 0) return;
    for (size_t i = 0; i < units && in[i] != 0 && o + 1 < out_len; i++) {
        u32 cp = in[i];
        if (cp < 0x80) {
            out[o++] = (char)cp;
        } else if (cp < 0x800 && o + 2 < out_len) {
            out[o++] = (char)(0xC0 | (cp >> 6));
            out[o++] = (char)(0x80 | (cp & 0x3F));
        } else if (o + 3 < out_len) {
            out[o++] = (char)(0xE0 | (cp >> 12));
            out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[o++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    out[o] = '\0';
    while (o > 0 && (out[o - 1] == ' ' || out[o - 1] == '\t')) out[--o] = '\0';
}

static int banner_langs(u16 version) {
    if (version == 0x0001) return 6;
    if (version == 0x0002) return 7;
    if (version == 0x0003 || version == 0x0103) return 8;
    return 0;
}

static int lang_index(u8 language) {
    switch (language) {
        case CFG_LANGUAGE_JP: return 0;
        case CFG_LANGUAGE_EN: return 1;
        case CFG_LANGUAGE_FR: return 2;
        case CFG_LANGUAGE_DE: return 3;
        case CFG_LANGUAGE_IT: return 4;
        case CFG_LANGUAGE_ES: return 5;
        case CFG_LANGUAGE_ZH: return 6;
        case CFG_LANGUAGE_KO: return 7;
        case CFG_LANGUAGE_TW: return 6;
        default: return 1;
    }
}

static bool bitmap_blank(const u8 *bitmap) {
    for (int i = 0; i < 0x200; i++) {
        if (bitmap[i] != 0) return false;
    }
    return true;
}

static void apply_banner(TitleRec *title, const u8 *banner, u32 got, const u8 *header, u8 language) {
    if (got < 0x240) return;
    u16 version = 0;
    memcpy(&version, banner, sizeof(version));
    int langs = banner_langs(version);
    if (langs == 0) return;

    u16 *icon = malloc(32 * 32 * sizeof(u16));
    if (icon) {
        decode_nds_icon(icon, banner + 0x20, (const u16 *)(banner + 0x220), false, false);
        free(title->icon);
        title->icon = icon;
        title->icon_w = 32;
    }

    if (version == 0x0103 && got >= BANNER_MAX) {
        const u16 *raw = (const u16 *)(banner + 0x2340);
        const u8 *bitmaps = banner + 0x1240;
        u16 seq[64];
        u8 len = 0;
        for (int i = 0; i < 64; i++) {
            u16 token = 0;
            memcpy(&token, raw + i, sizeof(token));
            if ((token & 0xFF) == 0) break;
            u8 bmp = (u8)((token >> 8) & 7);
            if (bitmap_blank(bitmaps + (size_t)bmp * 0x200)) continue;
            seq[len++] = token;
        }
        bool same = true;
        if (len > 1) {
            u16 key = (u16)(seq[0] & (u16)~0xFF);
            for (u8 i = 1; i < len; i++) {
                if ((u16)(seq[i] & (u16)~0xFF) != key) same = false;
            }
        }
        if (len >= 2 && !same) {
            IconAnim *anim = calloc(1, sizeof(IconAnim));
            if (anim) {
                memcpy(anim->bitmap, banner + 0x1240, sizeof(anim->bitmap));
                memcpy(anim->palette, banner + 0x2240, sizeof(anim->palette));
                memcpy(anim->seq, seq, len * sizeof(u16));
                anim->seq_len = len;
                free(title->anim);
                title->anim = anim;
            }
        }
    }

    int prefer = lang_index(language);
    if (prefer >= langs) prefer = langs > 1 ? 1 : 0;
    int order[8];
    int count = 0;
    order[count++] = prefer;
    if (langs > 1 && prefer != 1) order[count++] = 1;
    if (prefer != 0) order[count++] = 0;
    for (int i = 0; i < langs; i++) {
        bool seen = false;
        for (int n = 0; n < count; n++) {
            if (order[n] == i) seen = true;
        }
        if (!seen) order[count++] = i;
    }
    for (int n = 0; n < count; n++) {
        if (got < (u32)(0x240 + (order[n] + 1) * 0x100)) continue;
        char full[96];
        banner_to_utf8(full, sizeof(full), (const u16 *)(banner + 0x240 + order[n] * 0x100), 128);
        if (full[0] == '\0') continue;
        char *split = strpbrk(full, "\n\r");
        if (split) {
            *split = '\0';
            char *rest = split + 1;
            while (*rest == '\n' || *rest == '\r' || *rest == ' ') rest++;
            if (*rest) {
                snprintf(title->publisher, sizeof(title->publisher), "%.*s", (int)sizeof(title->publisher) - 1, rest);
            }
        }
        memcpy(title->name, full, sizeof(title->name));
        title->name[sizeof(title->name) - 1] = '\0';
        break;
    }

    if (title->product[0] == '\0' && header) {
        char code[5];
        memcpy(code, header + 0x0C, 4);
        code[4] = '\0';
        bool ok = false;
        for (int i = 0; i < 4; i++) {
            if ((unsigned char)code[i] > ' ') ok = true;
        }
        if (ok) snprintf(title->product, sizeof(title->product), "%s", code);
    }
}

static bool header_ok(const u8 *hdr, u32 n) {
    if (n < 0x70) return false;
    for (int i = 0; i < 12; i++) {
        u8 c = hdr[i];
        if (c != 0 && (c < 0x20 || c > 0x7E)) return false;
    }
    u32 off = 0;
    memcpy(&off, hdr + 0x68, sizeof(off));
    return off >= 0x200 && off <= 0x02000000;
}

static bool read_srl(const char *path, TitleRec *title, u8 language) {
    Handle file = 0;
    if (R_FAILED(open_file(path, &file))) return false;

    u8 header[0x200];
    u32 got = 0;
    Result rc = FSFILE_Read(file, &got, 0, header, sizeof(header));
    if (R_FAILED(rc) || !header_ok(header, got)) {
        FSFILE_Close(file);
        return false;
    }
    u32 banner_off = 0;
    memcpy(&banner_off, header + 0x68, sizeof(banner_off));

    u8 *banner = malloc(BANNER_MAX);
    u32 banner_got = 0;
    if (banner) {
        rc = FSFILE_Read(file, &banner_got, banner_off, banner, BANNER_MAX);
        if (R_FAILED(rc)) banner_got = 0;
    }
    FSFILE_Close(file);
    if (!banner || banner_got < 0x240) {
        free(banner);
        return false;
    }
    u16 version = 0;
    memcpy(&version, banner, sizeof(version));
    if (banner_langs(version) == 0) {
        free(banner);
        return false;
    }
    apply_banner(title, banner, banner_got, header, language);
    free(banner);
    return title->icon != NULL || title->name[0] != '\0';
}

static bool name_is_app(const u16 *name) {
    int n = 0;
    while (name[n] && n < 0x100) n++;
    if (n < 5 || name[n - 4] != '.') return false;
    char ext[3] = {
        (char)name[n - 3],
        (char)name[n - 2],
        (char)name[n - 1],
    };
    for (int i = 0; i < 3; i++) {
        if (ext[i] >= 'A' && ext[i] <= 'Z') ext[i] = (char)(ext[i] - 'A' + 'a');
    }
    return ext[0] == 'a' && ext[1] == 'p' && ext[2] == 'p';
}

static bool scan_content_dir(const char *dir, TitleRec *title, u8 language) {
    Handle handle = 0;
    if (R_FAILED(open_dir(dir, &handle))) return false;
    bool found = false;
    for (int n = 0; n < 8 && !found; n++) {
        FS_DirectoryEntry ent;
        u32 read = 0;
        if (R_FAILED(FSDIR_Read(handle, &read, 1, &ent)) || read != 1) break;
        if (!name_is_app(ent.name)) continue;
        char file[16];
        size_t i = 0;
        for (; ent.name[i] && i < sizeof(file) - 1; i++) {
            u16 c = ent.name[i];
            file[i] = c < 0x80 ? (char)c : '?';
        }
        file[i] = '\0';
        char path[96];
        snprintf(path, sizeof(path), "%s/%s", dir, file);
        if (read_srl(path, title, language)) found = true;
    }
    FSDIR_Close(handle);
    return found;
}

void dsi_fill(TitleRec *title, u8 language) {
    if (!archive_open()) return;

    u32 low = (u32)title->id;
    u32 highs[2];
    highs[0] = 0x00030004;
    highs[1] = 0x00048004;
    if ((u32)(title->id >> 32) == 0x00030004) {
        highs[0] = 0x00030004;
        highs[1] = 0x00048004;
    } else {
        highs[0] = 0x00048004;
        highs[1] = 0x00030004;
    }

    for (int i = 0; i < 2; i++) {
        char dir[64];
        snprintf(dir, sizeof(dir), "/title/%08lx/%08lx/content",
                 (unsigned long)highs[i], (unsigned long)low);
        if (scan_content_dir(dir, title, language)) return;
        snprintf(dir, sizeof(dir), "title/%08lx/%08lx/content",
                 (unsigned long)highs[i], (unsigned long)low);
        if (scan_content_dir(dir, title, language)) return;
    }
}
