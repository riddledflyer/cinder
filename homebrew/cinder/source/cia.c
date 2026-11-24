#include "cia.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static FS_Archive g_sd;
static bool g_sd_open;
static u8 *g_buf;
static u32 g_buf_size;

static u64 align64(u64 n) {
    return (n + 63ull) & ~63ull;
}

static void sd_close(void) {
    if (g_sd_open) {
        FSUSER_CloseArchive(g_sd);
        g_sd_open = false;
    }
}

static bool sd_open(void) {
    if (g_sd_open) return true;
    if (R_FAILED(FSUSER_OpenArchive(&g_sd, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")))) return false;
    g_sd_open = true;
    return true;
}

static Result open_path(const u16 *path, Handle *out) {
    return FSUSER_OpenFile(out, g_sd, fsMakePath(PATH_UTF16, path), FS_OPEN_READ, 0);
}

static void name_to_utf8(char *out, size_t out_len, const u16 *in, size_t units) {
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
}

static bool ends_cia(const u16 *name) {
    int n = 0;
    while (name[n] && n < 0x106) n++;
    if (n < 5 || name[0] == '.' || name[n - 4] != '.') return false;
    u16 ext[3] = {name[n - 3], name[n - 2], name[n - 1]};
    for (int i = 0; i < 3; i++) {
        if (ext[i] >= 'A' && ext[i] <= 'Z') ext[i] = (u16)(ext[i] - 'A' + 'a');
    }
    return ext[0] == 'c' && ext[1] == 'i' && ext[2] == 'a';
}

static bool build_path(u16 *out, int cap, const char *dir, const u16 *name) {
    int i = 0;
    for (const char *p = dir; *p; p++) {
        if (i + 1 >= cap) return false;
        out[i++] = (u16)(unsigned char)*p;
    }
    for (int n = 0; name[n]; n++) {
        if (i + 1 >= cap) return false;
        out[i++] = name[n];
    }
    out[i] = 0;
    return true;
}

static u32 bswap32(u32 v) {
    return (v >> 24) | ((v >> 8) & 0xFF00) | ((v << 8) & 0xFF0000) | (v << 24);
}

static u16 bswap16(u16 v) {
    return (u16)((v >> 8) | (v << 8));
}

static u64 bswap64(u64 v) {
    return ((u64)bswap32((u32)v) << 32) | bswap32((u32)(v >> 32));
}

static FS_MediaType media_of(u64 id) {
    u32 high = (u32)(id >> 32);
    if (high == 0x00048004 || high == 0x00030004 || titles_is_system_id(id)) return MEDIATYPE_NAND;
    return MEDIATYPE_SD;
}

static u32 sig_block(u32 type) {
    switch (type) {
        case 0x00010000:
        case 0x00010003:
            return 0x240;
        case 0x00010001:
        case 0x00010004:
            return 0x140;
        case 0x00010002:
        case 0x00010005:
            return 0x80;
        default:
            return 0;
    }
}

static bool read_info(Handle file, u64 file_size, u64 *declared, u64 *title_id, u16 *version) {
    u8 hdr[0x20];
    u32 got = 0;
    if (R_FAILED(FSFILE_Read(file, &got, 0, hdr, sizeof(hdr))) || got < sizeof(hdr)) return false;

    u32 header_size = 0, cert_size = 0, ticket_size = 0, tmd_size = 0, meta_size = 0;
    u64 content_size = 0;
    memcpy(&header_size, hdr + 0x00, 4);
    memcpy(&cert_size, hdr + 0x08, 4);
    memcpy(&ticket_size, hdr + 0x0C, 4);
    memcpy(&tmd_size, hdr + 0x10, 4);
    memcpy(&meta_size, hdr + 0x14, 4);
    memcpy(&content_size, hdr + 0x18, 8);

    if (header_size < 0x2020 || header_size > 0x3000) return false;
    if (cert_size == 0 || cert_size > 0x10000) return false;
    if (ticket_size < 0x200 || ticket_size > 0x2000) return false;
    if (tmd_size < 0x140 || tmd_size > 0x5000) return false;
    if (meta_size > 0x40000) return false;
    if (content_size == 0 || content_size > 0x100000000ull) return false;

    *declared = align64(header_size) + align64(cert_size) + align64(ticket_size) +
                align64(tmd_size) + align64(content_size) + align64(meta_size);
    *title_id = 0;
    *version = 0;

    u64 tmd_off = align64(header_size) + align64(cert_size) + align64(ticket_size);
    if (file_size < tmd_off + 4) return true;

    u32 sig = 0;
    got = 0;
    if (R_FAILED(FSFILE_Read(file, &got, tmd_off, &sig, 4)) || got != 4) return true;
    u32 block = sig_block(sig);
    if (block == 0) block = sig_block(bswap32(sig));
    if (block == 0 || file_size < tmd_off + block + 0x9E) return true;

    u8 body[0xA0];
    got = 0;
    if (R_FAILED(FSFILE_Read(file, &got, tmd_off + block, body, sizeof(body))) || got < sizeof(body)) return true;
    memcpy(title_id, body + 0x4C, 8);
    memcpy(version, body + 0x9C, 2);
    *title_id = bswap64(*title_id);
    *version = bswap16(*version);
    return true;
}

static void fill_meta(CiaRec *cia, Handle file, u64 file_size, u8 language) {
    u8 hdr[0x20];
    u32 got = 0;
    if (R_FAILED(FSFILE_Read(file, &got, 0, hdr, sizeof(hdr))) || got < sizeof(hdr)) return;

    u32 header_size = 0, cert_size = 0, ticket_size = 0, tmd_size = 0, meta_size = 0;
    u64 content_size = 0;
    memcpy(&header_size, hdr + 0x00, 4);
    memcpy(&cert_size, hdr + 0x08, 4);
    memcpy(&ticket_size, hdr + 0x0C, 4);
    memcpy(&tmd_size, hdr + 0x10, 4);
    memcpy(&meta_size, hdr + 0x14, 4);
    memcpy(&content_size, hdr + 0x18, 8);
    if (content_size > 0 && content_size < 0x100000000ull) cia->install_size = content_size;

    u64 meta_off = align64(header_size) + align64(cert_size) + align64(ticket_size) +
                   align64(tmd_size) + align64(content_size);

    u64 smdh_at = 0;
    bool have = false;
    if (meta_size >= 0x400 + 8 && file_size >= meta_off + 0x400 + 8) {
        smdh_at = meta_off + 0x400;
        have = true;
    } else if (meta_size >= 8 && file_size >= meta_off + 8) {
        smdh_at = meta_off;
        have = true;
    }
    if (have) {
        u8 *blob = malloc(0x36C0);
        u32 n = 0;
        if (blob) {
            u32 want = 0x36C0;
            u64 room = file_size - smdh_at;
            if (room < want) want = (u32)room;
            if (R_SUCCEEDED(FSFILE_Read(file, &got, smdh_at, blob, want)) && got >= 8 && memcmp(blob, "SMDH", 4) == 0) {
                n = got;
            } else if (smdh_at != meta_off && file_size >= meta_off + 8) {
                want = 0x36C0;
                room = file_size - meta_off;
                if (room < want) want = (u32)room;
                if (meta_size < want) want = meta_size;
                if (want >= 8 && R_SUCCEEDED(FSFILE_Read(file, &got, meta_off, blob, want)) && got >= 8 &&
                    memcmp(blob, "SMDH", 4) == 0) {
                    n = got;
                }
            }
        }
        if (n >= 8) {
            u16 *icon = NULL;
            u8 icon_w = 0;
            if (smdh_from_blob(blob, n, language,
                               cia->title_name, sizeof(cia->title_name),
                               cia->publisher, sizeof(cia->publisher),
                               &icon, &icon_w)) {
                cia->icon = icon;
                cia->icon_w = icon_w;
            }
        }
        free(blob);
    }

    if (cia->complete) {
        u64 need = 0;
        if (R_SUCCEEDED(AM_GetCiaRequiredSpace(&need, cia->media, file)) && need > 0) cia->install_size = need;
    }
}

static void fill_rec(CiaRec *cia, Handle file, u8 language) {
    u64 declared = 0, title = 0;
    u16 version = 0;
    u64 size = 0;
    FSFILE_GetSize(file, &size);
    cia->file_size = size;
    bool ok = read_info(file, size, &declared, &title, &version);
    cia->header_ok = ok;
    cia->declared_size = declared;
    cia->title_id = title;
    cia->version = version;
    cia->complete = ok && declared != 0 && size == declared;
    cia->kind = title ? titles_kind_of(title) : KIND_GAME;
    cia->media = media_of(title);

    cia->installable = cia->complete && (cia->kind != KIND_SYSTEM || titles_is_home_menu(title));
    cia->selected = false;
    if (ok) fill_meta(cia, file, size, language);
}

static int cmp_cia(const void *a, const void *b) {
    const CiaRec *ca = a;
    const CiaRec *cb = b;
    if (ca->file_size < cb->file_size) return 1;
    if (ca->file_size > cb->file_size) return -1;
    return strcmp(ca->name, cb->name);
}

static void scan_dir(CiaList *list, const char *dir_path, const char *prefix, u8 language) {
    if (list->count >= CIA_CAP) return;
    Handle handle = 0;
    if (R_FAILED(FSUSER_OpenDirectory(&handle, g_sd, fsMakePath(PATH_ASCII, dir_path)))) return;

    for (;;) {
        if (list->count >= CIA_CAP) break;
        FS_DirectoryEntry ent;
        u32 read = 0;
        if (R_FAILED(FSDIR_Read(handle, &read, 1, &ent)) || read != 1) break;
        if (ent.attributes & FS_ATTRIBUTE_DIRECTORY) continue;
        if (!ends_cia(ent.name)) continue;

        CiaRec *cia = &list->items[list->count];
        memset(cia, 0, sizeof(*cia));
        if (!build_path(cia->path, 160, prefix, ent.name)) continue;
        name_to_utf8(cia->name, sizeof(cia->name), ent.name, 0x106);

        Handle file = 0;
        if (R_FAILED(open_path(cia->path, &file))) continue;
        fill_rec(cia, file, language);
        FSFILE_Close(file);
        if (titles_hidden(cia->title_id)) {
            free(cia->icon);
            continue;
        }
        list->count++;
    }
    FSDIR_Close(handle);
}

Result cias_load(CiaList *list) {
    cias_free(list);
    memset(list, 0, sizeof(*list));
    if (!sd_open()) {
        list->rc = (Result)0xC8804471;
        return list->rc;
    }
    list->items = calloc(CIA_CAP, sizeof(CiaRec));
    if (!list->items) {
        list->rc = CIA_ERR_OOM;
        return list->rc;
    }
    u8 language = CFG_LANGUAGE_EN;
    if (R_SUCCEEDED(cfguInit())) {
        CFGU_GetSystemLanguage(&language);
        cfguExit();
    }
    u32 before = 0;
    scan_dir(list, "/cias/", "/cias/", language);
    if (list->count == before) scan_dir(list, "/cias", "/cias/", language);
    scan_dir(list, "/", "/", language);
    if (list->count > 1) qsort(list->items, list->count, sizeof(CiaRec), cmp_cia);
    if (list->count == 0) {
        free(list->items);
        list->items = NULL;
    } else if (list->count < CIA_CAP) {
        CiaRec *shrunk = realloc(list->items, list->count * sizeof(CiaRec));
        if (shrunk) list->items = shrunk;
    }
    return 0;
}

static bool hex_name(const char *name, u64 id) {
    char hex[17];
    if (!name || name[0] == '\0') return true;
    snprintf(hex, sizeof(hex), "%016llX", (unsigned long long)id);
    return strcmp(name, hex) == 0;
}

static u16 *dup_pix(const u16 *src, u8 w) {
    int n = w > 0 ? w : 48;
    if (n > 48) n = 48;
    u16 *pix = malloc((size_t)n * (size_t)n * sizeof(u16));
    if (!pix) return NULL;
    memcpy(pix, src, (size_t)n * (size_t)n * sizeof(u16));
    return pix;
}

static bool hex_digit(char c) {
    return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f') || (c >= 'A' && c <= 'F');
}

static void tidy_filename(char *dst, size_t n, const char *file) {
    const char *s = file ? file : "";
    int hex = 0;
    size_t o = 0;
    while (hex < 16 && hex_digit(s[hex])) hex++;
    if (hex == 16 && s[16] == ' ') s += 17;
    while (*s && o + 1 < n) {
        if (*s == '.' && (s[1] == 'c' || s[1] == 'C')) break;
        if (*s == ' ' && s[1] == '(') break;
        dst[o++] = *s++;
    }
    while (o > 0 && dst[o - 1] == ' ') o--;
    dst[o] = '\0';
}

static void take_name(char *dst, size_t n, const char *src) {
    if (!src || src[0] == '\0' || n == 0) return;
    snprintf(dst, n, "%s", src);
}

static void take_icon(u16 **dst, u8 *dst_w, const u16 *src, u8 src_w) {
    u16 *pix;
    if (*dst || !src) return;
    pix = dup_pix(src, src_w);
    if (!pix) return;
    *dst = pix;
    *dst_w = src_w > 0 ? src_w : 48;
}

static const TitleRec *installed_game(const TitleList *list, u32 low) {
    const TitleRec *fallback = NULL;
    if (!list || !list->items) return NULL;
    for (u32 i = 0; i < list->count; i++) {
        const TitleRec *title = &list->items[i];
        if ((u32)title->id != low) continue;
        if (title->kind != KIND_GAME && title->kind != KIND_DEMO) continue;
        if (hex_name(title->name, title->id)) {
            if (!fallback) fallback = title;
            continue;
        }
        if (title->kind == KIND_GAME) return title;
        if (!fallback) fallback = title;
    }
    return fallback;
}

static const TitleRec *installed_id(const TitleList *list, u64 id) {
    if (!list || !list->items || id == 0) return NULL;
    for (u32 i = 0; i < list->count; i++) {
        if (list->items[i].id == id) return &list->items[i];
    }
    return NULL;
}

static const CiaRec *cia_game(const CiaList *cias, u32 low, u32 skip) {
    const CiaRec *fallback = NULL;
    if (!cias->items) return NULL;
    for (u32 i = 0; i < cias->count; i++) {
        const CiaRec *cia = &cias->items[i];
        if (i == skip || (u32)cia->title_id != low) continue;
        if (cia->kind != KIND_GAME && cia->kind != KIND_DEMO) continue;
        if (cia->title_name[0] || cia->icon) return cia;
        if (!fallback) fallback = cia;
    }
    return fallback;
}

void cias_link(CiaList *cias, const TitleList *installed) {
    if (!cias || !cias->items) return;
    for (u32 i = 0; i < cias->count; i++) {
        CiaRec *cia = &cias->items[i];
        if (cia->kind == KIND_DLC) {
            const CiaRec *file = cia_game(cias, (u32)cia->title_id, i);
            const TitleRec *game = installed_game(installed, (u32)cia->title_id);
            if (file && file->title_name[0]) {
                take_name(cia->title_name, sizeof(cia->title_name), file->title_name);
                if (cia->publisher[0] == '\0') take_name(cia->publisher, sizeof(cia->publisher), file->publisher);
                take_icon(&cia->icon, &cia->icon_w, file->icon, file->icon_w);
            } else if (game && !hex_name(game->name, game->id)) {
                take_name(cia->title_name, sizeof(cia->title_name), game->name);
                if (cia->publisher[0] == '\0') take_name(cia->publisher, sizeof(cia->publisher), game->publisher);
                take_icon(&cia->icon, &cia->icon_w, game->icon, game->icon_w);
            }
        } else if (cia->kind == KIND_DSIWARE) {
            const TitleRec *have = installed_id(installed, cia->title_id);
            if (have && !hex_name(have->name, have->id)) {
                if (cia->title_name[0] == '\0') take_name(cia->title_name, sizeof(cia->title_name), have->name);
                if (cia->publisher[0] == '\0') take_name(cia->publisher, sizeof(cia->publisher), have->publisher);
                take_icon(&cia->icon, &cia->icon_w, have->icon, have->icon_w);
            }
        }
        if ((cia->kind == KIND_DLC || cia->kind == KIND_DSIWARE) && cia->title_name[0] == '\0') {
            tidy_filename(cia->title_name, sizeof(cia->title_name), cia->name);
        }
        cia->licensed = titles_is_licensed(cia->publisher);
    }
}

void cias_free(CiaList *list) {
    if (!list) return;
    if (list->items) {
        for (u32 i = 0; i < list->count; i++) free(list->items[i].icon);
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    sd_close();
    free(g_buf);
    g_buf = NULL;
    g_buf_size = 0;
}

static u8 *buffer(void) {
    if (g_buf) return g_buf;
    static const u32 sizes[] = {128 * 1024, 64 * 1024, 16 * 1024};
    for (int i = 0; i < 3; i++) {
        g_buf = malloc(sizes[i]);
        if (g_buf) {
            g_buf_size = sizes[i];
            return g_buf;
        }
    }
    return NULL;
}

static void close_import(Handle cia, bool cancel) {
    if (cancel) {
        if (R_FAILED(AM_CancelCIAInstall(cia))) svcCloseHandle(cia);
        return;
    }
}

#define AM_ALREADY ((Result)0xC8E083FC)

static bool title_installed(FS_MediaType media, u64 id) {
    AM_TitleInfo info;
    if (id == 0) return false;
    memset(&info, 0, sizeof(info));
    if (R_SUCCEEDED(AM_GetTitleInfo(media, 1, &id, &info)) && info.titleID == id) return true;
    memset(&info, 0, sizeof(info));
    if (R_SUCCEEDED(AM_GetTitleInfosIgnorePlatform(&info, &id, 1, media)) && info.titleID == id) return true;
    return false;
}

static void wipe_title(u64 id, bool pending_all) {
    if (id == 0) return;
    AM_DeletePendingTitle(MEDIATYPE_SD, id);
    AM_DeletePendingTitle(MEDIATYPE_NAND, id);
    AM_DeleteUserTitle(MEDIATYPE_SD, id);
    AM_DeleteUserTitle(MEDIATYPE_NAND, id);
    AM_DeleteTitle(MEDIATYPE_SD, id);
    AM_DeleteTitle(MEDIATYPE_NAND, id);
    AM_DeleteTicket(id);
    if (pending_all) {
        u32 flags = AM_PENDING_NON_SYSTEM | AM_PENDING_SYSTEM;
        AM_DeleteAllPendingTitles(MEDIATYPE_SD);
        AM_DeleteAllPendingTitles(MEDIATYPE_NAND);
        AM_DeleteAllImportTitleContexts(MEDIATYPE_SD, flags);
        AM_DeleteAllImportTitleContexts(MEDIATYPE_NAND, flags);
    }
    AM_DoCleanup(MEDIATYPE_SD);
    AM_DoCleanup(MEDIATYPE_NAND);
}

static FS_MediaType choose_media(u64 id) {
    u32 high = (u32)(id >> 32);
    if (title_installed(MEDIATYPE_SD, id)) return MEDIATYPE_SD;
    if (title_installed(MEDIATYPE_NAND, id)) return MEDIATYPE_NAND;
    if (high == 0x00048004 || high == 0x00030004 || titles_is_system_id(id)) return MEDIATYPE_NAND;
    return MEDIATYPE_SD;
}

static u64 cia_title_id(Handle file, u64 parsed) {
    AM_TitleInfo info;
    memset(&info, 0, sizeof(info));
    if (R_SUCCEEDED(AM_GetCiaFileInfo(MEDIATYPE_SD, &info, file)) && info.titleID) return info.titleID;
    memset(&info, 0, sizeof(info));
    if (R_SUCCEEDED(AM_GetCiaFileInfo(MEDIATYPE_NAND, &info, file)) && info.titleID) return info.titleID;
    return parsed;
}

static Result begin_import(FS_MediaType media, u64 id, bool self, bool hard, Handle *import) {
    if (self || titles_is_home_menu(id)) {
        if (id) AM_DeletePendingTitle(media, id);
        Result over = AM_StartCiaInstallOverwrite(import, media);
        if (R_SUCCEEDED(over)) return over;
        return AM_StartCiaInstall(media, import);
    }

    wipe_title(id, hard);
    if (hard) {
        Result over = AM_StartCiaInstallOverwrite(import, media);
        if (R_SUCCEEDED(over)) return over;
    }
    Result rc = AM_StartCiaInstall(media, import);
    if (rc != AM_ALREADY) return rc;
    Result over = AM_StartCiaInstallOverwrite(import, media);
    if (R_SUCCEEDED(over)) return over;
    wipe_title(id, true);
    if (hard) {
        over = AM_StartCiaInstallOverwrite(import, media);
        if (R_SUCCEEDED(over)) return over;
    }
    return AM_StartCiaInstall(media, import);
}

Result cias_install_one(const CiaRec *cia, u32 index, u32 total, CiaPumpFn pump, void *user, bool replace) {
    if (!cia) return CIA_ERR_TRUNCATED;
    if ((cia->kind == KIND_SYSTEM || titles_is_system_id(cia->title_id)) && !titles_is_home_menu(cia->title_id))
        return CIA_ERR_SYSTEM;
    if (!cia->installable || !cia->complete) return CIA_ERR_TRUNCATED;
    if (!sd_open()) return (Result)0xC8804471;

    Handle file = 0;
    Result rc = open_path(cia->path, &file);
    if (R_FAILED(rc)) return rc;

    u64 size = 0;
    FSFILE_GetSize(file, &size);
    u64 declared = 0, parsed = 0;
    u16 version = 0;
    bool ok = read_info(file, size, &declared, &parsed, &version);
    if (!ok || declared == 0 || size != declared || (cia->title_id != 0 && parsed != 0 && parsed != cia->title_id)) {
        FSFILE_Close(file);
        return CIA_ERR_TRUNCATED;
    }

    u64 title = cia_title_id(file, parsed);
    u8 *buf = buffer();
    if (!buf) {
        FSFILE_Close(file);
        return CIA_ERR_OOM;
    }

    bool self = title == CINDER_TITLE_ID || title == titles_self_id();
    FS_MediaType media = title ? choose_media(title) : cia->media;

    for (int attempt = 0; attempt < (replace ? 1 : 2); attempt++) {
        Handle import = 0;
        rc = begin_import(media, title, self, replace || attempt == 1, &import);
        if (R_FAILED(rc)) {
            FSFILE_Close(file);
            return rc;
        }

        u64 off = 0;
        bool cancel = false;
        while (off < size) {
            if (pump && !pump(cia, index, total, off, user)) {
                cancel = true;
                break;
            }
            u32 chunk = g_buf_size;
            if ((u64)chunk > size - off) chunk = (u32)(size - off);
            u32 got = 0;
            rc = FSFILE_Read(file, &got, off, buf, chunk);
            if (R_FAILED(rc) || got == 0) break;
            u32 wrote = 0;
            rc = FSFILE_Write(import, &wrote, off, buf, got, 0);
            if (R_FAILED(rc) || wrote != got) {
                if (R_SUCCEEDED(rc)) rc = CIA_ERR_TRUNCATED;
                break;
            }
            off += wrote;
        }

        if (cancel || off != size || R_FAILED(rc)) {
            close_import(import, true);
            FSFILE_Close(file);
            if (cancel) return CIA_ERR_CANCEL;
            return R_FAILED(rc) ? rc : CIA_ERR_TRUNCATED;
        }
        if (pump) pump(cia, index, total, size, user);

        rc = AM_FinishCiaInstall(import);
        if (replace || rc != AM_ALREADY || attempt == 1 || self) {

            FSFILE_Close(file);
            return rc;
        }
    }

    FSFILE_Close(file);
    return rc;
}
