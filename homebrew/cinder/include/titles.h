#pragma once

#ifdef __cplusplus
extern "C" {
#endif

#include <3ds.h>
#include <stdbool.h>
#include <stddef.h>

#define CINDER_TITLE_ID 0x0004000000C1D400ULL
#define TITLE_CAP 1024
#define DELETE_CHUNK 16

typedef enum {
    KIND_GAME = 0,
    KIND_DEMO,
    KIND_UPDATE,
    KIND_DLC,
    KIND_DSIWARE,
    KIND_SYSTEM
} TitleKind;

typedef struct {
    u8 bitmap[8][0x200];
    u16 palette[8][16];
    u16 seq[64];
    u8 seq_len;
} IconAnim;

typedef struct {
    u64 id;
    FS_MediaType media;
    u64 size;
    u16 version;
    TitleKind kind;
    bool deletable;
    bool selected;
    char name[96];
    char publisher[64];
    bool licensed;
    char product[17];

    u16 *icon;
    u8 icon_w;
    bool ready;
    bool anim_tried;

    IconAnim *anim;
} TitleRec;

typedef struct {
    TitleRec *items;
    u32 count;
    FS_MediaType media;
} TitleList;

u64 titles_self_id(void);

void titles_set_load_pump(void (*fn)(void));
void titles_warm(TitleRec *title);

Result titles_load(TitleList *list, FS_MediaType media);

Result titles_load_all(TitleList *list);
void titles_free(TitleList *list);

const char *titles_kind_name(TitleKind kind);
char titles_kind_char(TitleKind kind);

bool titles_is_system_id(u64 id);
bool titles_is_home_menu(u64 id);
bool titles_hidden(u64 id);
bool titles_is_licensed(const char *publisher);
TitleKind titles_kind_of(u64 id);
void titles_format_size(char *out, size_t n, u64 bytes);
void titles_format_version(char *out, size_t n, u16 version);
void smdh_fill_names(TitleRec *title, u8 language);

bool smdh_from_blob(const u8 *blob, u32 got, u8 language,
                    char *name, size_t name_len,
                    char *publisher, size_t pub_len,
                    u16 **icon, u8 *icon_w);

void dsi_fill(TitleRec *title, u8 language);
void dsi_shutdown(void);

u16 titles_anim_token(const IconAnim *anim, u32 tick);
bool titles_anim_frame(const IconAnim *anim, u32 tick, u16 *dst);

u32 titles_collect(const TitleList *list, u64 *out, u32 cap, bool include_related);

typedef void (*TitleProgressFn)(u32 index, u32 total, u64 id, Result rc, bool ticket_ok, void *user);

Result titles_delete(FS_MediaType media, const u64 *ids, u32 count, bool tickets,
                     TitleProgressFn progress, void *user);

#ifdef __cplusplus
}
#endif
