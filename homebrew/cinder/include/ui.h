#pragma once

#include <3ds.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

void ui_init(void);
void ui_exit(void);
void ui_drop_images(void);
void ui_screen(gfxScreen_t screen);
void ui_present(void);
void *ui_image(const u16 *pix, int src_w, bool key_zero);

#ifdef __cplusplus
}
#endif
