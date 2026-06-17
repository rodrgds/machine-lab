#include "keyboard_lib.h"

static uint8_t g_bytes[2];
static uint8_t g_size;
static int g_ready;
static int g_make;

int kbc_read_status(uint8_t *status) {
  return lcom_port_read8(KBC_ST_REG, status);
}

int kbc_read_output(uint8_t *byte) {
  return lcom_port_read8(KBC_OUT_BUF, byte);
}

int kbc_write_command(uint8_t command) {
  return lcom_port_write8(KBC_CMD_REG, command);
}

int kbd_process_byte(uint8_t byte) {
  g_ready = 0;
  if (g_size == 0 && byte == KBD_TWO_BYTE_PFX) {
    g_bytes[0] = byte;
    g_size = 1;
    return 0;
  }
  if (g_size == 1) {
    g_bytes[1] = byte;
    g_size = 2;
  } else {
    g_bytes[0] = byte;
    g_size = 1;
  }
  g_make = (byte & KBD_BREAK_BIT) == 0;
  g_ready = 1;
  return 1;
}

int kbd_get_scancode(uint8_t bytes[2], uint8_t *size, int *make) {
  if (!g_ready || bytes == 0 || size == 0 || make == 0) return LCOM_ERR;
  bytes[0] = g_bytes[0];
  bytes[1] = g_bytes[1];
  *size = g_size;
  *make = g_make;
  return LCOM_OK;
}
