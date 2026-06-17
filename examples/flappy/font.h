#ifndef FLAPPY_FONT_H
#define FLAPPY_FONT_H

#include <lcom/lcom.h>

#include <stdint.h>

typedef struct {
  uint8_t *fb;
  lcom_vbe_mode_info_t info;
} draw_context_t;

uint32_t font_text_width(const char *text, uint8_t scale);
void draw_text(draw_context_t *ctx, int x, int y, const char *text, uint32_t color, uint8_t scale);

#endif
