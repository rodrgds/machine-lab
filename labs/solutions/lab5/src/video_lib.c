#include "video_lib.h"

#include <stdio.h>
#include <string.h>

static lcom_vbe_mode_info_t g_info;
static uint8_t *g_fb;

static uint32_t parse_xpm_color(const char *value, int *transparent) {
  *transparent = 0;
  if (strcmp(value, "None") == 0) {
    *transparent = 1;
    return 0;
  }
  if (value[0] != '#') return 0xFFFFFFu;
  unsigned r = 0, g = 0, b = 0;
  if (sscanf(value + 1, "%02x%02x%02x", &r, &g, &b) != 3) return 0xFFFFFFu;
  return (b & 0xFFu) | ((g & 0xFFu) << 8) | ((r & 0xFFu) << 16);
}

int video_set_mode(uint16_t mode) {
  if (lcom_vbe_get_mode_info(mode, &g_info) != LCOM_OK) return LCOM_ERR;
  return lcom_vbe_set_mode(mode);
}

int video_map_framebuffer(void) {
  return lcom_phys_map(g_info.framebuffer_phys, g_info.framebuffer_size, (void **)&g_fb);
}

int video_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
  if (g_fb == 0) return LCOM_ERR;
  for (uint16_t yy = y; yy < y + h && yy < g_info.height; yy++) {
    for (uint16_t xx = x; xx < x + w && xx < g_info.width; xx++) {
      uint8_t *px = g_fb + (uint32_t)yy * g_info.pitch + (uint32_t)xx * g_info.bytes_per_pixel;
      px[0] = (uint8_t)(color & 0xFFu);
      px[1] = (uint8_t)((color >> 8) & 0xFFu);
      px[2] = (uint8_t)((color >> 16) & 0xFFu);
    }
  }
  return LCOM_OK;
}

int video_draw_xpm(const char *const *xpm, int16_t x, int16_t y) {
  if (g_fb == 0 || xpm == 0) return LCOM_ERR;
  int width = 0, height = 0, colors = 0, cpp = 0;
  if (sscanf(xpm[0], "%d %d %d %d", &width, &height, &colors, &cpp) != 4) return LCOM_ERR;
  if (cpp != 1 || width <= 0 || height <= 0 || colors <= 0) return LCOM_ERR;

  uint32_t table[256] = {0};
  int transparent[256] = {0};
  for (int i = 0; i < colors; i++) {
    unsigned char key = (unsigned char)xpm[1 + i][0];
    const char *marker = strstr(xpm[1 + i], " c ");
    if (marker == 0) return LCOM_ERR;
    table[key] = parse_xpm_color(marker + 3, &transparent[key]);
  }

  for (int yy = 0; yy < height; yy++) {
    const char *row = xpm[1 + colors + yy];
    for (int xx = 0; xx < width; xx++) {
      int dst_x = x + xx;
      int dst_y = y + yy;
      if (dst_x < 0 || dst_y < 0 || dst_x >= g_info.width || dst_y >= g_info.height) continue;
      unsigned char key = (unsigned char)row[xx];
      if (transparent[key]) continue;
      uint8_t *px = g_fb + (uint32_t)dst_y * g_info.pitch + (uint32_t)dst_x * g_info.bytes_per_pixel;
      uint32_t color = table[key];
      px[0] = (uint8_t)(color & 0xFFu);
      px[1] = (uint8_t)((color >> 8) & 0xFFu);
      px[2] = (uint8_t)((color >> 16) & 0xFFu);
    }
  }
  return LCOM_OK;
}

int video_present(void) {
  return lcom_vbe_present();
}
