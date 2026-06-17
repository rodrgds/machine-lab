#include <lcom/i8042.h>
#include <lcom/lcom.h>

#include <stdbool.h>
#include <stdint.h>

static bool is_break(uint8_t byte) {
  return (byte & KBD_BREAK_BIT) != 0;
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;

  lcom_irq_t irq;
  if (lcom_irq_subscribe(KBC_IRQ, 0, &irq) != LCOM_OK) return 1;

  bool done = false;
  while (!done) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) return 1;
    if ((ev.irq_mask & irq.mask) == 0) continue;

    uint8_t status = 0;
    uint8_t data = 0;
    if (lcom_port_read8(KBC_ST_REG, &status) != LCOM_OK) return 1;
    if ((status & KBC_ST_OBF) == 0 || (status & KBC_ST_AUX) != 0) continue;
    if (lcom_port_read8(KBC_OUT_BUF, &data) != LCOM_OK) return 1;

    lcom_printf("kbd %s 0x%02x\n", is_break(data) ? "break" : "make", data);
    if (data == ESC_BREAK) done = true;
  }

  lcom_irq_unsubscribe(&irq);
  lcom_exit();
  return 0;
}
