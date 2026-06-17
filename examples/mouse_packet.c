#include <lcom/i8042.h>
#include <lcom/lcom.h>

#include <stdint.h>

static int read_mouse_byte(lcom_irq_t irq, uint8_t *out) {
  for (;;) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) return 1;
    if ((ev.irq_mask & irq.mask) == 0) continue;

    uint8_t status = 0;
    if (lcom_port_read8(KBC_ST_REG, &status) != LCOM_OK) return 1;
    if ((status & KBC_ST_OBF) == 0 || (status & KBC_ST_AUX) == 0) continue;
    if (lcom_port_read8(KBC_OUT_BUF, out) != LCOM_OK) return 1;
    return 0;
  }
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;

  lcom_irq_t irq;
  if (lcom_irq_subscribe(MOUSE_IRQ, 0, &irq) != LCOM_OK) return 1;

  if (lcom_port_write8(KBC_CMD_REG, KBC_CMD_WRITE_MOUSE) != LCOM_OK) return 1;
  if (lcom_port_write8(KBC_IN_BUF, MOUSE_CMD_ENABLE_DR) != LCOM_OK) return 1;

  uint8_t ack = 0;
  if (read_mouse_byte(irq, &ack) != 0 || ack != MOUSE_ACK) return 1;

  uint8_t bytes[3];
  for (int i = 0; i < 3; i++) {
    if (read_mouse_byte(irq, &bytes[i]) != 0) return 1;
  }

  int16_t dx = (bytes[0] & BIT(4)) ? (int16_t)(0xFF00u | bytes[1]) : (int16_t)bytes[1];
  int16_t dy = (bytes[0] & BIT(5)) ? (int16_t)(0xFF00u | bytes[2]) : (int16_t)bytes[2];
  lcom_printf("mouse packet %02x %02x %02x dx=%d dy=%d lb=%u\n",
              bytes[0], bytes[1], bytes[2], dx, dy, (bytes[0] & BIT(0)) ? 1 : 0);

  lcom_irq_unsubscribe(&irq);
  lcom_exit();
  return 0;
}
