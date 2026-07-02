#ifndef LAB5_VIDEO_LIB_H
#define LAB5_VIDEO_LIB_H

#include <lcom/vbe.h>

#include <stdint.h>

int video_set_mode(uint16_t mode);
int video_map_framebuffer(void);
int video_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color);
int video_draw_xpm(const char *const *xpm, int16_t x, int16_t y);
int video_present(void);

#endif
