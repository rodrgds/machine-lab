#include "mouse_lib.h"

static uint8_t g_buf[3];
static uint8_t g_idx;
static int g_ready;

static int send_mouse_command(uint8_t command) {
  if (lcom_port_write8(KBC_CMD_REG, KBC_CMD_WRITE_MOUSE) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_write8(KBC_IN_BUF, command) != LCOM_OK) return LCOM_ERR;
  return LCOM_OK;
}

int mouse_enable_data_reporting(void) {
  return send_mouse_command(MOUSE_CMD_ENABLE_DR);
}

int mouse_disable_data_reporting(void) {
  return send_mouse_command(MOUSE_CMD_DISABLE_DR);
}

int mouse_process_byte(uint8_t byte) {
  g_ready = 0;
  if (g_idx == 0 && (byte & MOUSE_SYNC_BIT) == 0) return 0;
  g_buf[g_idx++] = byte;
  if (g_idx == 3) {
    g_idx = 0;
    g_ready = 1;
    return 1;
  }
  return 0;
}

int mouse_get_packet(mouse_packet_t *packet) {
  if (!g_ready || packet == 0) return LCOM_ERR;
  packet->bytes[0] = g_buf[0];
  packet->bytes[1] = g_buf[1];
  packet->bytes[2] = g_buf[2];
  packet->lb = (g_buf[0] & BIT(0)) != 0;
  packet->rb = (g_buf[0] & BIT(1)) != 0;
  packet->mb = (g_buf[0] & BIT(2)) != 0;
  packet->dx = (g_buf[0] & BIT(4)) ? (int16_t)(0xFF00u | g_buf[1]) : (int16_t)g_buf[1];
  packet->dy = (g_buf[0] & BIT(5)) ? (int16_t)(0xFF00u | g_buf[2]) : (int16_t)g_buf[2];
  return LCOM_OK;
}
