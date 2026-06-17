#include <lcom/i8042.h>
#include <lcom/i8254.h>
#include <lcom/lcom.h>
#include <lcom/vbe.h>

#include <stdint.h>

static void put_pixel(uint8_t *fb, const lcom_vbe_mode_info_t *info,
                      uint16_t x, uint16_t y, uint8_t r, uint8_t g, uint8_t b) {
  if (x >= info->width || y >= info->height || info->bytes_per_pixel < 3) return;
  uint8_t *px = fb + (uint32_t)y * info->pitch + (uint32_t)x * info->bytes_per_pixel;
  px[0] = b;
  px[1] = g;
  px[2] = r;
}

static void fill_rect(uint8_t *fb, const lcom_vbe_mode_info_t *info,
                      uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                      uint8_t r, uint8_t g, uint8_t b) {
  for (uint16_t yy = y; yy < y + h && yy < info->height; yy++) {
    for (uint16_t xx = x; xx < x + w && xx < info->width; xx++) {
      put_pixel(fb, info, xx, yy, r, g, b);
    }
  }
}

static void draw_scene(uint8_t *fb, const lcom_vbe_mode_info_t *info, uint32_t frame) {
  fill_rect(fb, info, 0, 0, info->width, info->height, 24, 30, 36);
  fill_rect(fb, info, 52, 64, 250, 120, 34, 139, 168);
  fill_rect(fb, info, 86, 214, 360, 42, 235, 181, 85);
  fill_rect(fb, info, 520, 96, 160, 260, 98, 188, 112);

  uint16_t x = (uint16_t)(100 + (frame * 3u) % 560u);
  fill_rect(fb, info, x, 410, 56, 56, 232, 93, 78);
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;
  lcom_printf("SDL demo: virtual VBE + PIT + i8042. Press ESC to exit.\n");
  lcom_printf("Use F3 for the runtime debug menu and F11 for fullscreen.\n");

  lcom_vbe_mode_info_t info;
  if (lcom_vbe_get_mode_info(LCOM_VBE_MODE_800_600_24, &info) != LCOM_OK) return 1;
  if (lcom_vbe_set_mode(LCOM_VBE_MODE_800_600_24) != LCOM_OK) return 1;

  uint8_t *fb = 0;
  if (lcom_phys_map(info.framebuffer_phys, info.framebuffer_size, (void **)&fb) != LCOM_OK) return 1;

  lcom_irq_t timer_irq;
  lcom_irq_t kbd_irq;
  if (lcom_irq_subscribe(TIMER0_IRQ, 0, &timer_irq) != LCOM_OK) return 1;
  if (lcom_irq_subscribe(KBC_IRQ, 0, &kbd_irq) != LCOM_OK) return 1;

  uint32_t frame = 0;
  uint8_t running = 1;
  while (running && frame < 3600) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) break;

    if (ev.irq_mask & timer_irq.mask) {
      draw_scene(fb, &info, frame++);
      lcom_vbe_present();
    }

    if (ev.irq_mask & kbd_irq.mask) {
      uint8_t status = 0;
      uint8_t data = 0;
      if (lcom_port_read8(KBC_ST_REG, &status) == LCOM_OK &&
          (status & KBC_ST_OBF) != 0 &&
          (status & KBC_ST_AUX) == 0 &&
          lcom_port_read8(KBC_OUT_BUF, &data) == LCOM_OK) {
        lcom_printf("kbd byte 0x%02x\n", data);
        if (data == ESC_BREAK) running = 0;
      }
    }
  }

  lcom_irq_unsubscribe(&kbd_irq);
  lcom_irq_unsubscribe(&timer_irq);
  lcom_phys_unmap(fb, info.framebuffer_size);
  lcom_exit();
  return 0;
}
