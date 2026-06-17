#include "renderer.h"

#include <stdio.h>
#include <string.h>

static void put_pixel(draw_context_t *ctx, int x, int y, uint32_t color) {
  if (x < 0 || y < 0 || x >= ctx->info.width || y >= ctx->info.height) return;
  uint8_t *px = ctx->fb + (uint32_t)y * ctx->info.pitch + (uint32_t)x * ctx->info.bytes_per_pixel;
  px[0] = (uint8_t)(color & 0xFFu);
  px[1] = (uint8_t)((color >> 8) & 0xFFu);
  px[2] = (uint8_t)((color >> 16) & 0xFFu);
}

void clear_screen(draw_context_t *ctx, uint32_t color) {
  fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height, color);
}

void fill_rect(draw_context_t *ctx, int x, int y, int w, int h, uint32_t color) {
  for (int yy = y; yy < y + h; yy++) {
    for (int xx = x; xx < x + w; xx++) {
      put_pixel(ctx, xx, yy, color);
    }
  }
}

static uint32_t parse_color(const char *s, int *transparent) {
  *transparent = 0;
  if (strcmp(s, "None") == 0) {
    *transparent = 1;
    return 0;
  }
  if (s[0] != '#') return 0xFFFFFFu;
  unsigned r = 0, g = 0, b = 0;
  sscanf(s + 1, "%02x%02x%02x", &r, &g, &b);
  return (b & 0xFFu) | ((g & 0xFFu) << 8) | ((r & 0xFFu) << 16);
}

void draw_xpm(draw_context_t *ctx, const char *const *xpm, int x, int y, int scale) {
  int w = 0, h = 0, colors = 0, cpp = 0;
  sscanf(xpm[0], "%d %d %d %d", &w, &h, &colors, &cpp);
  if (cpp != 1 || scale <= 0) return;

  uint32_t table[256] = {0};
  int transparent[256] = {0};
  for (int i = 0; i < colors; i++) {
    unsigned char key = (unsigned char)xpm[1 + i][0];
    const char *c = strstr(xpm[1 + i], " c ");
    if (c == 0) continue;
    table[key] = parse_color(c + 3, &transparent[key]);
  }

  for (int yy = 0; yy < h; yy++) {
    const char *row = xpm[1 + colors + yy];
    for (int xx = 0; xx < w; xx++) {
      unsigned char key = (unsigned char)row[xx];
      if (transparent[key]) continue;
      for (int sy = 0; sy < scale; sy++) {
        for (int sx = 0; sx < scale; sx++) {
          put_pixel(ctx, x + xx * scale + sx, y + yy * scale + sy, table[key]);
        }
      }
    }
  }
}
