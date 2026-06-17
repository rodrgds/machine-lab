#include <lcom/lcom.h>
#include <lcom/vbe.h>

#include <stdint.h>

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;

  lcom_vbe_mode_info_t info;
  if (lcom_vbe_get_mode_info(LCOM_VBE_MODE_800_600_24, &info) != LCOM_OK) return 1;
  if (lcom_vbe_set_mode(LCOM_VBE_MODE_800_600_24) != LCOM_OK) return 1;

  uint8_t *fb = 0;
  if (lcom_phys_map(info.framebuffer_phys, info.framebuffer_size, (void **)&fb) != LCOM_OK) return 1;

  for (uint16_t y = 80; y < 180; y++) {
    for (uint16_t x = 100; x < 260; x++) {
      uint8_t *px = fb + (uint32_t)y * info.pitch + (uint32_t)x * info.bytes_per_pixel;
      px[0] = 0x20;
      px[1] = 0x80;
      px[2] = 0xFF;
    }
  }

  lcom_vbe_present();
  lcom_printf("rectangle drawn %ux%u bpp=%u\n", info.width, info.height, info.bpp);
  lcom_phys_unmap(fb, info.framebuffer_size);
  lcom_exit();
  return 0;
}
