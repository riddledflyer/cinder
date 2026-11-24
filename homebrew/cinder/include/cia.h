#pragma once

#include "titles.h"

#ifdef __cplusplus
extern "C" {
#endif

#define CIA_CAP 256
#define CIA_ERR_TRUNCATED ((Result)0xC8E04401)
#define CIA_ERR_SYSTEM ((Result)0xC8E04402)
#define CIA_ERR_OOM ((Result)0xC8E04403)
#define CIA_ERR_CANCEL ((Result)0xC8E04404)

typedef struct {
    u16 path[160];
    char name[64];
    u64 file_size;
    u64 declared_size;
    u64 title_id;
    u16 version;
    TitleKind kind;
    FS_MediaType media;
    bool header_ok;
    bool complete;
    bool installable;
    bool selected;
    u64 install_size;
    char title_name[96];
    char publisher[64];
    bool licensed;
    u16 *icon;
    u8 icon_w;
} CiaRec;

typedef struct {
    CiaRec *items;
    u32 count;
    Result rc;
} CiaList;

Result cias_load(CiaList *list);
void cias_link(CiaList *cias, const TitleList *installed);
void cias_free(CiaList *list);

typedef bool (*CiaPumpFn)(const CiaRec *cia, u32 index, u32 total, u64 wrote, void *user);

Result cias_install_one(const CiaRec *cia, u32 index, u32 total, CiaPumpFn pump, void *user, bool replace);

#ifdef __cplusplus
}
#endif
