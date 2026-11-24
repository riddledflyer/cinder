#include "titles.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define TITLE_ARCHIVE ((FS_ArchiveID)0x2345678A)

typedef struct {
    u64 title_id;
    u8 media;
    u8 pad[7];
} TitleArchivePath;

typedef struct {
    u32 access_type;
    u32 content_index;
    u32 section;
    char name[8];
} TitleFilePath;

typedef struct {
    u16 short_desc[0x40];
    u16 long_desc[0x80];
    u16 publisher[0x40];
} SmdhTitle;

typedef struct {
    char magic[4];
    u16 version;
    u16 reserved;
    SmdhTitle titles[16];
} SmdhHead;

_Static_assert(sizeof(TitleArchivePath) == 0x10, "archive lowpath");
_Static_assert(sizeof(TitleFilePath) == 0x14, "file lowpath");
_Static_assert(sizeof(SmdhHead) == 0x2008, "smdh head");

static void smdh_utf16_to_utf8(char *out, size_t out_len, const u16 *in, size_t units) {
    size_t o = 0;
    if (out_len == 0) return;
    for (size_t i = 0; i < units && in[i] != 0 && o + 1 < out_len; i++) {
        u32 cp = in[i];
        if (cp >= 0xD800 && cp <= 0xDBFF && i + 1 < units) {
            u32 lo = in[i + 1];
            if (lo >= 0xDC00 && lo <= 0xDFFF) {
                cp = 0x10000 + (((cp - 0xD800) << 10) | (lo - 0xDC00));
                i++;
            }
        }
        if (cp < 0x80) {
            out[o++] = (char)cp;
        } else if (cp < 0x800 && o + 2 < out_len) {
            out[o++] = (char)(0xC0 | (cp >> 6));
            out[o++] = (char)(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000 && o + 3 < out_len) {
            out[o++] = (char)(0xE0 | (cp >> 12));
            out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[o++] = (char)(0x80 | (cp & 0x3F));
        } else if (o + 4 < out_len) {
            out[o++] = (char)(0xF0 | (cp >> 18));
            out[o++] = (char)(0x80 | ((cp >> 12) & 0x3F));
            out[o++] = (char)(0x80 | ((cp >> 6) & 0x3F));
            out[o++] = (char)(0x80 | (cp & 0x3F));
        }
    }
    out[o] = '\0';
    while (o > 0 && (out[o - 1] == ' ' || out[o - 1] == '\t')) {
        out[--o] = '\0';
    }
}

static bool name_empty(const char *text) {
    return text[0] == '\0';
}

static Result read_smdh(FS_MediaType media, u64 title_id, u8 *out, u32 out_len, u32 *got_out) {
    TitleArchivePath arch;
    memset(&arch, 0, sizeof(arch));
    arch.title_id = title_id;
    arch.media = (u8)media;

    TitleFilePath file;
    memset(&file, 0, sizeof(file));
    file.access_type = 0;
    file.content_index = 0;
    file.section = 2;
    memcpy(file.name, "icon", 4);

    FS_Path arch_path = {PATH_BINARY, (u32)sizeof(arch), &arch};
    FS_Path file_path = {PATH_BINARY, (u32)sizeof(file), &file};

    Handle handle = 0;
    Result rc = FSUSER_OpenFileDirectly(&handle, TITLE_ARCHIVE, arch_path, file_path, FS_OPEN_READ, 0);
    if (R_FAILED(rc)) {
        memset(file.name, 0, sizeof(file.name));
        rc = FSUSER_OpenFileDirectly(&handle, TITLE_ARCHIVE, arch_path, file_path, FS_OPEN_READ, 0);
    }
    if (R_FAILED(rc)) return rc;

    u32 got = 0;
    rc = FSFILE_Read(handle, &got, 0, out, out_len);
    FSFILE_Close(handle);
    if (got_out) *got_out = got;
    if (R_FAILED(rc)) return rc;
    if (got < 8 || memcmp(out, "SMDH", 4) != 0) return (Result)0xC8804478;
    return 0;
}

static int tile_z(int x, int y) {
    int z = 0;
    for (int i = 0; i < 3; i++) {
        z |= ((x >> i) & 1) << (2 * i);
        z |= ((y >> i) & 1) << (2 * i + 1);
    }
    return z;
}

static void decode_tiled(u16 *dst, int dst_w, const u16 *src, int tiles) {
    for (int ty = 0; ty < tiles; ty++) {
        for (int tx = 0; tx < tiles; tx++) {
            const u16 *tile = src + ((ty * tiles + tx) * 64);
            for (int y = 0; y < 8; y++) {
                for (int x = 0; x < 8; x++) {
                    dst[(ty * 8 + y) * dst_w + (tx * 8 + x)] = tile[tile_z(x, y)];
                }
            }
        }
    }
}

static void decode_icon(u16 *dst, const u8 *blob, u32 got) {
    if (got >= 0x24C0 + 0x1200) {
        decode_tiled(dst, 48, (const u16 *)(blob + 0x24C0), 6);
        return;
    }
    if (got < 0x2040 + 0x480) return;
    u16 small[24 * 24];
    decode_tiled(small, 24, (const u16 *)(blob + 0x2040), 3);
    for (int y = 0; y < 48; y++) {
        int sy = y * 24 / 48;
        for (int x = 0; x < 48; x++) {
            dst[y * 48 + x] = small[sy * 24 + (x * 24 / 48)];
        }
    }
}

bool smdh_from_blob(const u8 *blob, u32 got, u8 language,
                    char *name, size_t name_len,
                    char *publisher, size_t pub_len,
                    u16 **icon, u8 *icon_w) {
    if (name && name_len) name[0] = '\0';
    if (publisher && pub_len) publisher[0] = '\0';
    if (icon) *icon = NULL;
    if (icon_w) *icon_w = 0;
    if (!blob || got < 8 || memcmp(blob, "SMDH", 4) != 0) return false;

    const SmdhHead *head = (const SmdhHead *)blob;
    if (got >= sizeof(SmdhHead)) {
        u8 order[16];
        u8 count = 0;
        if (language < 16) order[count++] = language;
        if (language != 1) order[count++] = 1;
        for (u8 i = 0; i < 12; i++) {
            if (i != language && i != 1) order[count++] = i;
        }
        for (u8 n = 0; n < count; n++) {
            char short_name[96];
            smdh_utf16_to_utf8(short_name, sizeof(short_name), head->titles[order[n]].short_desc, 0x40);
            if (name_empty(short_name)) continue;
            if (name && name_len) snprintf(name, name_len, "%s", short_name);
            if (publisher && pub_len) {
                smdh_utf16_to_utf8(publisher, pub_len, head->titles[order[n]].publisher, 0x40);
            }
            break;
        }
    }
    if (icon && got >= 0x2040 + 0x480) {
        u16 *pixels = malloc(48 * 48 * sizeof(u16));
        if (pixels) {
            decode_icon(pixels, blob, got);
            *icon = pixels;
            if (icon_w) *icon_w = 48;
        }
    }
    return true;
}

void smdh_fill_names(TitleRec *title, u8 language) {
    title->name[0] = '\0';
    title->publisher[0] = '\0';
    title->product[0] = '\0';
    free(title->icon);
    title->icon = NULL;
    title->icon_w = 0;
    free(title->anim);
    title->anim = NULL;

    if (title->kind == KIND_DSIWARE) {
        dsi_fill(title, language);
    } else {
        static u8 blob[0x36C0];
        u32 got = 0;
        if (R_SUCCEEDED(read_smdh(title->media, title->id, blob, 0x36C0, &got))) {
            char name[96];
            char publisher[64];
            u16 *icon = NULL;
            u8 icon_w = 0;
            if (smdh_from_blob(blob, got, language, name, sizeof(name), publisher, sizeof(publisher), &icon, &icon_w)) {
                if (!name_empty(name)) snprintf(title->name, sizeof(title->name), "%s", name);
                if (!name_empty(publisher)) snprintf(title->publisher, sizeof(title->publisher), "%s", publisher);
                title->icon = icon;
                title->icon_w = icon_w;
            }
        }
    }

    char product[17];
    memset(product, 0, sizeof(product));
    if (R_SUCCEEDED(AM_GetTitleProductCode(title->media, title->id, product))) {
        product[16] = '\0';
        bool any = false;
        for (int i = 0; i < 16; i++) {
            if ((unsigned char)product[i] > ' ') any = true;
        }
        if (any) snprintf(title->product, sizeof(title->product), "%s", product);
    }

    if (name_empty(title->name)) {
        if (!name_empty(title->product)) snprintf(title->name, sizeof(title->name), "%s", title->product);
        else snprintf(title->name, sizeof(title->name), "%016llX", (unsigned long long)title->id);
    }
    if (name_empty(title->publisher)) snprintf(title->publisher, sizeof(title->publisher), "-");
}
