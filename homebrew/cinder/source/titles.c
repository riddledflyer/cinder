#include "titles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

_Static_assert(TITLE_CAP <= 1024, "title cap");
_Static_assert(
    sizeof(TitleRec) * TITLE_CAP + sizeof(u64) * TITLE_CAP + sizeof(AM_TitleInfo) * TITLE_CAP < 1024 * 1024,
    "title load must stay under 1MB of the 4MB heap");

static void (*g_load_pump)(void);

u64 titles_self_id(void) {
    s64 info = 0;

    if (R_SUCCEEDED(svcGetProcessInfo(&info, CUR_PROCESS_HANDLE, 0x10001)) && info != 0) {
        return (u64)info;
    }
    return CINDER_TITLE_ID;
}

bool titles_is_system_id(u64 id) {

    switch ((u32)(id >> 32)) {
        case 0x00040010:
        case 0x0004001B:
        case 0x00040030:
        case 0x0004009B:
        case 0x000400DB:
        case 0x00040130:
        case 0x00040138:
        case 0x0004013F:
        case 0x00030005:
        case 0x0003000F:
        case 0x00030015:
        case 0x00030017:
        case 0x00048005:
        case 0x0004800F:
            return true;
        default:
            return false;
    }
}

bool titles_is_home_menu(u64 id) {
    if ((u32)(id >> 32) != 0x00040030) return false;
    switch ((u32)id) {
        case 0x00008202:
        case 0x00008F02:
        case 0x00009802:
        case 0x0000A102:
        case 0x0000A902:
        case 0x0000B102:
            return true;
        default:
            return false;
    }
}

TitleKind titles_kind_of(u64 id) {
    switch ((u32)(id >> 32)) {
        case 0x00040000: return KIND_GAME;
        case 0x00040002: return KIND_DEMO;
        case 0x0004000E: return KIND_UPDATE;
        case 0x0004008C: return KIND_DLC;
        case 0x00048004:
        case 0x00030004: return KIND_DSIWARE;
        default: return titles_is_system_id(id) ? KIND_SYSTEM : KIND_GAME;
    }
}

bool titles_hidden(u64 id) {
    return id == 0x0004008C00008F05ULL || id == 0x0004008C00008F06ULL;
}

static int cmp_title(const void *a, const void *b) {
    const TitleRec *ta = a;
    const TitleRec *tb = b;
    if (ta->size < tb->size) return 1;
    if (ta->size > tb->size) return -1;
    return strcmp(ta->name, tb->name);
}

const char *titles_kind_name(TitleKind kind) {
    switch (kind) {
        case KIND_GAME: return "Game";
        case KIND_DEMO: return "Demo";
        case KIND_UPDATE: return "Update";
        case KIND_DLC: return "DLC";
        case KIND_DSIWARE: return "NDS";
        default: return "System";
    }
}

char titles_kind_char(TitleKind kind) {
    switch (kind) {
        case KIND_GAME: return 'G';
        case KIND_DEMO: return 'M';
        case KIND_UPDATE: return 'U';
        case KIND_DLC: return 'D';
        case KIND_DSIWARE: return 'W';
        default: return 'S';
    }
}

void titles_format_size(char *out, size_t n, u64 bytes) {
    const unsigned long long b = (unsigned long long)bytes;
    if (b < 1024ull) {
        snprintf(out, n, "%lluB", b);
    } else if (b < 1024ull * 1024ull) {
        snprintf(out, n, "%lluKB", (b + 1023ull) / 1024ull);
    } else if (b < 1024ull * 1024ull * 1024ull) {
        unsigned long long tenths = (b * 10ull) / (1024ull * 1024ull);
        snprintf(out, n, "%llu.%lluMB", tenths / 10ull, tenths % 10ull);
    } else {
        unsigned long long tenths = (b * 10ull) / (1024ull * 1024ull * 1024ull);
        snprintf(out, n, "%llu.%lluGB", tenths / 10ull, tenths % 10ull);
    }
}

void titles_format_version(char *out, size_t n, u16 version) {
    unsigned major = (version >> 10) & 0x3F;
    unsigned minor = (version >> 4) & 0x3F;
    unsigned micro = version & 0xF;
    snprintf(out, n, "%u.%u.%u", major, minor, micro);
}

Result titles_load(TitleList *list, FS_MediaType media) {
    memset(list, 0, sizeof(*list));
    list->media = media;

    u32 count = 0;
    Result rc = AM_GetTitleCount(media, &count);
    if (R_FAILED(rc) || count == 0) return rc;
    if (count > TITLE_CAP) count = TITLE_CAP;

    u64 *ids = calloc(count, sizeof(u64));
    AM_TitleInfo *info = calloc(count, sizeof(AM_TitleInfo));
    TitleRec *items = calloc(count, sizeof(TitleRec));
    if (!ids || !info || !items) {
        free(ids);
        free(info);
        free(items);
        return (Result)0xC86044D2;
    }

    u32 read = 0;
    rc = AM_GetTitleList(&read, media, count, ids);
    if (R_SUCCEEDED(rc)) rc = AM_GetTitleInfo(media, read, ids, info);
    if (R_FAILED(rc) || read == 0) {
        free(ids);
        free(info);
        free(items);
        return rc;
    }

    u64 self = titles_self_id();
    u32 kept = 0;
    for (u32 i = 0; i < read; i++) {
        u64 id = info[i].titleID != 0 ? info[i].titleID : ids[i];
        TitleKind kind = titles_kind_of(id);
        if (kind == KIND_SYSTEM) continue;
        if (id == self || id == CINDER_TITLE_ID || titles_hidden(id)) continue;

        TitleRec *title = &items[kept++];
        memset(title, 0, sizeof(*title));
        title->id = id;
        title->media = media;
        title->size = info[i].size;
        title->version = info[i].version;
        title->kind = kind;
        title->selected = false;
        title->deletable = true;
        snprintf(title->name, sizeof(title->name), "%016llX", (unsigned long long)id);
        snprintf(title->publisher, sizeof(title->publisher), "-");
    }

    free(ids);
    free(info);
    if (kept == 0) {
        free(items);
        return 0;
    }
    if (kept < read) {
        TitleRec *shrunk = realloc(items, kept * sizeof(TitleRec));
        if (shrunk) items = shrunk;
    }
    list->items = items;
    list->count = kept;
    return 0;
}

void titles_free(TitleList *list) {
    if (list->items) {
        for (u32 i = 0; i < list->count; i++) {
            free(list->items[i].icon);
            free(list->items[i].anim);
        }
    }
    free(list->items);
    list->items = NULL;
    list->count = 0;
    dsi_shutdown();
}

static void absorb(TitleList *dst, TitleList *src) {
    if (!src->items || src->count == 0) {
        titles_free(src);
        return;
    }
    TitleRec *merged = realloc(dst->items, (dst->count + src->count) * sizeof(TitleRec));
    if (!merged) {
        titles_free(src);
        return;
    }
    memcpy(merged + dst->count, src->items, src->count * sizeof(TitleRec));
    dst->items = merged;
    dst->count += src->count;
    free(src->items);
    src->items = NULL;
    src->count = 0;
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

static const TitleRec *parent_game(const TitleList *list, u32 low) {
    const TitleRec *fallback = NULL;
    if (!list->items) return NULL;
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

static void titles_link_dlc(TitleList *list) {
    if (!list->items) return;
    for (u32 i = 0; i < list->count; i++) {
        TitleRec *dlc = &list->items[i];
        const TitleRec *game;
        if (dlc->kind != KIND_DLC) continue;
        game = parent_game(list, (u32)dlc->id);
        if (!game || hex_name(game->name, game->id)) continue;
        snprintf(dlc->name, sizeof(dlc->name), "%s", game->name);
        if (dlc->publisher[0] == '\0' || strcmp(dlc->publisher, "-") == 0) {
            snprintf(dlc->publisher, sizeof(dlc->publisher), "%s", game->publisher);
        }
        if (dlc->product[0] == '\0') snprintf(dlc->product, sizeof(dlc->product), "%s", game->product);
        if (!dlc->icon && game->icon) {
            u16 *pix = dup_pix(game->icon, game->icon_w);
            if (pix) {
                dlc->icon = pix;
                dlc->icon_w = game->icon_w > 0 ? game->icon_w : 48;
            }
        }
    }
}

static u8 g_lang = CFG_LANGUAGE_EN;

static void grab_language(void) {
    if (R_SUCCEEDED(cfguInit())) {
        CFGU_GetSystemLanguage(&g_lang);
        cfguExit();
    }
}

void titles_warm(TitleRec *title) {
    if (!title || title->kind != KIND_DSIWARE || title->anim || title->anim_tried) return;
    title->anim_tried = true;
    dsi_fill(title, g_lang);
}

typedef struct {
    u64 id;
    u16 version;
    u8 media;
    u8 icon_w;
    char name[96];
    char publisher[64];
    char product[17];
    u8 pad;
} CacheRec;

static bool sd_archive(FS_Archive *out) {
    return R_SUCCEEDED(FSUSER_OpenArchive(out, ARCHIVE_SDMC, fsMakePath(PATH_EMPTY, "")));
}

static FS_Path utf16_path(u16 *buf, const char *text) {
    u32 n = 0;
    for (; text[n] && n < 80; n++) buf[n] = (u16)(unsigned char)text[n];
    buf[n] = 0;
    FS_Path path;
    path.type = PATH_UTF16;
    path.size = (n + 1) * 2;
    path.data = buf;
    return path;
}

static bool cache_apply(TitleList *list, u32 *file_count) {
    *file_count = 0;
    if (!list->items || list->count == 0) return false;
    FS_Archive arch = 0;
    if (!sd_archive(&arch)) return false;
    u16 wide[96];
    FS_Path path = utf16_path(wide, "/cinder/cache.bin");
    Handle file = 0;
    if (R_FAILED(FSUSER_OpenFile(&file, arch, path, FS_OPEN_READ, 0))) {
        FSUSER_CloseArchive(arch);
        return false;
    }
    u64 size = 0;
    FSFILE_GetSize(file, &size);
    u8 *buf = NULL;
    u32 got = 0;
    if (size >= 8 && size <= 2 * 1024 * 1024) buf = malloc((size_t)size);
    if (!buf || R_FAILED(FSFILE_Read(file, &got, 0, buf, (u32)size)) || got != (u32)size) {
        free(buf);
        FSFILE_Close(file);
        FSUSER_CloseArchive(arch);
        return false;
    }
    FSFILE_Close(file);
    FSUSER_CloseArchive(arch);
    if (memcmp(buf, "CND1", 4) != 0) {
        free(buf);
        return false;
    }
    u32 count = 0;
    memcpy(&count, buf + 4, 4);
    if (count > 4096) {
        free(buf);
        return false;
    }
    *file_count = count;
    u32 hits = 0;
    u32 off = 8;
    for (u32 n = 0; n < count; n++) {
        if (off + sizeof(CacheRec) > size) break;
        CacheRec rec;
        memcpy(&rec, buf + off, sizeof(rec));
        off += sizeof(rec);
        rec.name[sizeof(rec.name) - 1] = '\0';
        rec.publisher[sizeof(rec.publisher) - 1] = '\0';
        rec.product[sizeof(rec.product) - 1] = '\0';
        u16 *pix = NULL;
        if (rec.icon_w == 32 || rec.icon_w == 48) {
            u32 bytes = (u32)rec.icon_w * rec.icon_w * sizeof(u16);
            if (off + bytes > size) break;
            pix = malloc(bytes);
            if (!pix) break;
            memcpy(pix, buf + off, bytes);
            off += bytes;
        } else if (rec.icon_w != 0) {
            break;
        }
        TitleRec *match = NULL;
        for (u32 i = 0; i < list->count; i++) {
            TitleRec *title = &list->items[i];
            if (title->id == rec.id && (u8)title->media == rec.media && title->version == rec.version) {
                match = title;
                break;
            }
        }
        if (!match) {
            free(pix);
            continue;
        }
        snprintf(match->name, sizeof(match->name), "%s", rec.name);
        snprintf(match->publisher, sizeof(match->publisher), "%s", rec.publisher[0] ? rec.publisher : "-");
        snprintf(match->product, sizeof(match->product), "%s", rec.product);
        free(match->icon);
        match->icon = pix;
        match->icon_w = pix ? rec.icon_w : 0;
        match->ready = true;
        hits++;
    }
    free(buf);
    return hits == list->count && *file_count == list->count;
}

static void cache_write(const TitleList *list) {
    if (!list->items || list->count == 0) return;
    u64 bytes = 8;
    for (u32 i = 0; i < list->count; i++) {
        u8 w = list->items[i].icon_w;
        bytes += sizeof(CacheRec);
        if (list->items[i].icon && (w == 32 || w == 48)) bytes += (u64)w * w * sizeof(u16);
    }
    FS_Archive arch = 0;
    if (!sd_archive(&arch)) return;
    u16 dir_wide[96];
    u16 file_wide[96];
    FS_Path dir = utf16_path(dir_wide, "/cinder");
    FS_Path path = utf16_path(file_wide, "/cinder/cache.bin");
    FSUSER_CreateDirectory(arch, dir, 0);
    FSUSER_DeleteFile(arch, path);
    if (R_FAILED(FSUSER_CreateFile(arch, path, 0, bytes))) {
        FSUSER_CloseArchive(arch);
        return;
    }
    Handle file = 0;
    if (R_FAILED(FSUSER_OpenFile(&file, arch, path, FS_OPEN_WRITE, 0))) {
        FSUSER_CloseArchive(arch);
        return;
    }
    u32 wrote = 0;
    u64 off = 0;
    char magic[4] = {'C', 'N', 'D', '1'};
    u32 count = list->count;
    bool ok = R_SUCCEEDED(FSFILE_Write(file, &wrote, off, magic, 4, 0)) && wrote == 4;
    off += 4;
    if (ok) {
        ok = R_SUCCEEDED(FSFILE_Write(file, &wrote, off, &count, 4, 0)) && wrote == 4;
        off += 4;
    }
    for (u32 i = 0; ok && i < list->count; i++) {
        const TitleRec *title = &list->items[i];
        CacheRec rec;
        memset(&rec, 0, sizeof(rec));
        rec.id = title->id;
        rec.version = title->version;
        rec.media = (u8)title->media;
        if (title->icon && (title->icon_w == 32 || title->icon_w == 48)) rec.icon_w = title->icon_w;
        snprintf(rec.name, sizeof(rec.name), "%s", title->name);
        snprintf(rec.publisher, sizeof(rec.publisher), "%s", title->publisher);
        snprintf(rec.product, sizeof(rec.product), "%s", title->product);
        ok = R_SUCCEEDED(FSFILE_Write(file, &wrote, off, &rec, sizeof(rec), 0)) && wrote == sizeof(rec);
        off += sizeof(rec);
        if (!ok || rec.icon_w == 0) continue;
        u32 pixels = (u32)rec.icon_w * rec.icon_w * sizeof(u16);
        ok = R_SUCCEEDED(FSFILE_Write(file, &wrote, off, title->icon, pixels, 0)) && wrote == pixels;
        off += pixels;
    }
    if (ok) FSFILE_Write(file, &wrote, 0, magic, 4, FS_WRITE_FLUSH);
    FSFILE_Close(file);
    if (!ok) FSUSER_DeleteFile(arch, path);
    FSUSER_CloseArchive(arch);
}

static u32 g_fills;

static void fill_one(TitleRec *title) {
    if (g_load_pump && (g_fills % 8u) == 0) g_load_pump();
    g_fills++;
    smdh_fill_names(title, g_lang);
    title->ready = true;
}

Result titles_load_all(TitleList *list) {
    TitleList sd;
    TitleList nand;
    memset(&sd, 0, sizeof(sd));
    memset(&nand, 0, sizeof(nand));
    grab_language();
    g_fills = 0;
    Result sd_rc = titles_load(&sd, MEDIATYPE_SD);
    Result nand_rc = titles_load(&nand, MEDIATYPE_NAND);
    memset(list, 0, sizeof(*list));
    if (R_SUCCEEDED(sd_rc)) absorb(list, &sd);
    else titles_free(&sd);
    if (R_SUCCEEDED(nand_rc)) absorb(list, &nand);
    else titles_free(&nand);
    u32 file_count = 0;
    bool clean = cache_apply(list, &file_count);
    if (list->items) {
        for (u32 i = 0; i < list->count; i++) {
            TitleRec *title = &list->items[i];
            if (title->ready || title->kind == KIND_DLC) continue;
            fill_one(title);
        }
        titles_link_dlc(list);
        for (u32 i = 0; i < list->count; i++) {
            TitleRec *title = &list->items[i];
            if (title->ready) continue;
            if (title->kind == KIND_DLC && !hex_name(title->name, title->id)) {
                title->ready = true;
                continue;
            }
            fill_one(title);
        }
        if (list->count > 1) qsort(list->items, list->count, sizeof(TitleRec), cmp_title);
        for (u32 i = 0; i < list->count; i++)
            list->items[i].licensed = titles_is_licensed(list->items[i].publisher);
        if (!clean) cache_write(list);
    }
    if (R_FAILED(sd_rc) && R_FAILED(nand_rc)) return sd_rc;
    return 0;
}

void titles_set_load_pump(void (*fn)(void)) {
    g_load_pump = fn;
}

static bool id_in(const u64 *ids, u32 count, u64 id) {
    for (u32 i = 0; i < count; i++) {
        if (ids[i] == id) return true;
    }
    return false;
}

u32 titles_collect(const TitleList *list, u64 *out, u32 cap, bool include_related) {
    u32 count = 0;
    if (!list->items || cap == 0) return 0;

    for (u32 i = 0; i < list->count && count < cap; i++) {
        const TitleRec *title = &list->items[i];
        if (title->selected && title->deletable) out[count++] = title->id;
    }

    if (!include_related) return count;

    for (u32 i = 0; i < list->count; i++) {
        const TitleRec *base = &list->items[i];
        if (!base->selected || !base->deletable) continue;
        if (base->kind != KIND_GAME && base->kind != KIND_DEMO) continue;
        u32 low = (u32)base->id;
        for (u32 j = 0; j < list->count && count < cap; j++) {
            const TitleRec *other = &list->items[j];
            if (!other->deletable || other->media != base->media) continue;
            if (other->kind != KIND_UPDATE && other->kind != KIND_DLC) continue;
            if ((u32)other->id != low) continue;
            if (id_in(out, count, other->id)) continue;
            out[count++] = other->id;
        }
    }
    return count;
}

Result titles_delete(FS_MediaType media, const u64 *ids, u32 count, bool tickets,
                     TitleProgressFn progress, void *user) {
    u32 done = 0;
    Result last_fail = 0;
    bool any_fail = false;

    while (done < count) {
        if (!aptMainLoop()) break;
        u32 n = count - done;
        if (n > DELETE_CHUNK) n = DELETE_CHUNK;

        Result batch = AM_DeleteUserTitles((u64 *)(ids + done), n, media);
        if (R_SUCCEEDED(batch)) {
            for (u32 i = 0; i < n; i++) {
                bool ticket_ok = true;
                if (tickets) ticket_ok = R_SUCCEEDED(AM_DeleteTicket(ids[done + i]));
                if (progress) progress(done + i, count, ids[done + i], 0, ticket_ok, user);
            }
        } else {
            for (u32 i = 0; i < n; i++) {
                Result one = AM_DeleteUserTitle(media, ids[done + i]);
                bool ticket_ok = true;
                if (R_SUCCEEDED(one) && tickets) ticket_ok = R_SUCCEEDED(AM_DeleteTicket(ids[done + i]));
                if (R_FAILED(one)) {
                    any_fail = true;
                    last_fail = one;
                }
                if (progress) progress(done + i, count, ids[done + i], one, ticket_ok, user);
            }
        }
        done += n;
    }

    return any_fail ? last_fail : 0;
}
