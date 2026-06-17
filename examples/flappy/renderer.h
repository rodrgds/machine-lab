#ifndef FLAPPY_RENDERER_H
#define FLAPPY_RENDERER_H

#include "font.h"

#include <stdint.h>

void clear_screen(draw_context_t *ctx, uint32_t color);
void fill_rect(draw_context_t *ctx, int x, int y, int w, int h, uint32_t color);
void draw_xpm(draw_context_t *ctx, const char *const *xpm, int x, int y, int scale);

#endif
