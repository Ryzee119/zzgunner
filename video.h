// SPDX-License-Identifier: MIT
#ifndef _VIDEO_H
#define _VIDEO_H

#ifdef __cplusplus
extern "C" {
#endif

#ifdef N64
#include "ugfx.h"
#endif

#ifndef NUM_BUFFERS
#define NUM_BUFFERS 2
#endif

void video_init();
void video_begin_frame();
int video_get_width();
int video_get_height();
void video_wait_complete();
ugfx_buffer_t *video_get_display_list();
uint64_t video_get_render_mode();
void video_end_frame();
void video_set_render_mode(uint64_t mode);
void video_draw_tex_rect(void *tex, uint8_t bpp, int x, int y, int w, int h, int w_scale, int h_scale, bool mirror_x, bool mirror_y);
void video_draw_rect(uint8_t r, uint8_t g, uint8_t b, uint8_t a, int x, int y, int w, int h);
void video_shake(const int8_t *shake_array_x, const int8_t *shake_array_y, int frames, bool loop);
#ifdef __cplusplus
}
#endif

#endif