#include "app.h"
#include "sound.h"

#include <lcom/i8042.h>
#include <lcom/i8254.h>
#include <lcom/lcom.h>
#include <lcom/vbe.h>

#include <stdint.h>

typedef struct {
  uint8_t bytes[3];
  uint8_t index;
  int x;
  int y;
} mouse_state_t;

enum {
  INPUT_BYTES_PER_EVENT = 8
};

static void apply_input_result(app_t *app, app_input_result_t result) {
  if (result == APP_INPUT_FLAP) {
    sound_flap();
  } else if (result == APP_INPUT_QUIT) {
    app->running = 0;
  }
}

static int enable_mouse_reporting(void) {
  if (lcom_port_write8(KBC_CMD_REG, KBC_CMD_WRITE_MOUSE) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_write8(KBC_IN_BUF, MOUSE_CMD_ENABLE_DR) != LCOM_OK) return LCOM_ERR;
  return LCOM_OK;
}

static void disable_mouse_reporting(void) {
  lcom_port_write8(KBC_CMD_REG, KBC_CMD_WRITE_MOUSE);
  lcom_port_write8(KBC_IN_BUF, MOUSE_CMD_DISABLE_DR);
}

static app_input_result_t process_mouse_byte(app_t *app, mouse_state_t *mouse, uint8_t byte) {
  if (byte == MOUSE_ACK || byte == MOUSE_NACK || byte == MOUSE_ERR) return APP_INPUT_NONE;
  if (mouse->index == 0 && (byte & MOUSE_SYNC_BIT) == 0) return APP_INPUT_NONE;

  mouse->bytes[mouse->index++] = byte;
  if (mouse->index < 3) return APP_INPUT_NONE;
  mouse->index = 0;

  int dx = (mouse->bytes[0] & BIT(4)) ? (int)(int16_t)(0xFF00u | mouse->bytes[1]) : mouse->bytes[1];
  int dy = (mouse->bytes[0] & BIT(5)) ? (int)(int16_t)(0xFF00u | mouse->bytes[2]) : mouse->bytes[2];
  mouse->x += dx;
  mouse->y -= dy;
  if (mouse->x < 0) mouse->x = 0;
  if (mouse->y < 0) mouse->y = 0;
  if (mouse->x > 799) mouse->x = 799;
  if (mouse->y > 599) mouse->y = 599;

  return app_mouse(app, mouse->x, mouse->y, (mouse->bytes[0] & BIT(0)) != 0);
}

static int handle_i8042_byte(app_t *app, mouse_state_t *mouse) {
  uint8_t status = 0;
  uint8_t data = 0;
  if (lcom_port_read8(KBC_ST_REG, &status) != LCOM_OK) return 0;
  if ((status & KBC_ST_OBF) == 0) return 0;
  if (lcom_port_read8(KBC_OUT_BUF, &data) != LCOM_OK) return 0;
  if ((status & KBC_ST_ERR) != 0) return 1;

  if ((status & KBC_ST_AUX) != 0) {
    apply_input_result(app, process_mouse_byte(app, mouse, data));
  } else {
    apply_input_result(app, app_key(app, data));
  }
  return 1;
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;
  lcom_printf("LCOM Bird: SPACE/UP or mouse click flaps, ESC exits.\n");

  lcom_vbe_mode_info_t info;
  if (lcom_vbe_get_mode_info(LCOM_VBE_MODE_800_600_24, &info) != LCOM_OK) return 1;
  if (lcom_vbe_set_mode(LCOM_VBE_MODE_800_600_24) != LCOM_OK) return 1;

  uint8_t *fb = 0;
  if (lcom_phys_map(info.framebuffer_phys, info.framebuffer_size, (void **)&fb) != LCOM_OK) return 1;
  draw_context_t ctx = {fb, info};

  sound_init();

  lcom_irq_t timer_irq;
  lcom_irq_t kbd_irq;
  lcom_irq_t mouse_irq;
  if (lcom_irq_subscribe(TIMER0_IRQ, 0, &timer_irq) != LCOM_OK) return 1;
  if (lcom_irq_subscribe(KBC_IRQ, 0, &kbd_irq) != LCOM_OK) return 1;
  if (lcom_irq_subscribe(MOUSE_IRQ, 0, &mouse_irq) != LCOM_OK) return 1;

  app_t app;
  app_init(&app);
  mouse_state_t mouse = {{0, 0, 0}, 0, app.menu.pointer_x, app.menu.pointer_y};
  if (enable_mouse_reporting() != LCOM_OK) return 1;

  while (app.running) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) break;

    if (ev.irq_mask & (kbd_irq.mask | mouse_irq.mask)) {
      for (int i = 0; app.running && i < INPUT_BYTES_PER_EVENT; i++) {
        if (!handle_i8042_byte(&app, &mouse)) break;
      }
    }

    if (app.running && (ev.irq_mask & timer_irq.mask)) {
      app_update(&app);
      app_render(&app, &ctx);
      lcom_vbe_present();
    }
  }

  disable_mouse_reporting();
  lcom_irq_unsubscribe(&mouse_irq);
  lcom_irq_unsubscribe(&kbd_irq);
  lcom_irq_unsubscribe(&timer_irq);
  lcom_phys_unmap(fb, info.framebuffer_size);
  lcom_exit();
  return 0;
}
