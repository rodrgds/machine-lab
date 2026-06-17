#include "renderer.h"

#include <stdio.h>
#include <string.h>

static void put_pixel_unchecked(draw_context_t *ctx, int x, int y, uint32_t color) {
  uint8_t *px = ctx->fb + (uint32_t)y * ctx->info.pitch + (uint32_t)x * ctx->info.bytes_per_pixel;
  px[0] = (uint8_t)(color & 0xFFu);
  px[1] = (uint8_t)((color >> 8) & 0xFFu);
  px[2] = (uint8_t)((color >> 16) & 0xFFu);
}

static void put_pixel(draw_context_t *ctx, int x, int y, uint32_t color) {
  if (x < 0 || y < 0 || x >= ctx->info.width || y >= ctx->info.height) return;
  put_pixel_unchecked(ctx, x, y, color);
}

void clear_screen(draw_context_t *ctx, uint32_t color) {
  fill_rect(ctx, 0, 0, ctx->info.width, ctx->info.height, color);
}

void fill_rect(draw_context_t *ctx, int x, int y, int w, int h, uint32_t color) {
  if (w <= 0 || h <= 0) return;
  int x0 = x < 0 ? 0 : x;
  int y0 = y < 0 ? 0 : y;
  int x1 = x + w > ctx->info.width ? ctx->info.width : x + w;
  int y1 = y + h > ctx->info.height ? ctx->info.height : y + h;
  if (x0 >= x1 || y0 >= y1) return;

  uint8_t b = (uint8_t)(color & 0xFFu);
  uint8_t g = (uint8_t)((color >> 8) & 0xFFu);
  uint8_t r = (uint8_t)((color >> 16) & 0xFFu);
  uint32_t bpp = ctx->info.bytes_per_pixel;

  for (int yy = y0; yy < y1; yy++) {
    uint8_t *px = ctx->fb + (uint32_t)yy * ctx->info.pitch + (uint32_t)x0 * bpp;
    for (int xx = x0; xx < x1; xx++) {
      px[0] = b;
      px[1] = g;
      px[2] = r;
      px += bpp;
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
      if (scale == 1) {
        put_pixel(ctx, x + xx, y + yy, table[key]);
      } else {
        fill_rect(ctx, x + xx * scale, y + yy * scale, scale, scale, table[key]);
      }
    }
  }
}
