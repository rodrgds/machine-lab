#include <lcom/lcom.h>
#include <lcom/uart16550.h>

#include <stdint.h>

static int wait_rx(lcom_irq_t irq, uint8_t *out) {
  for (;;) {
    lcom_event_t ev = {0};
    if (lcom_event_wait(&ev) != LCOM_OK) return 1;
    if ((ev.irq_mask & irq.mask) == 0) continue;
    uint8_t lsr = 0;
    if (lcom_port_read8(COM1_BASE + SER_LSR, &lsr) != LCOM_OK) return 1;
    if ((lsr & LSR_RX_RDY) == 0) continue;
    return lcom_port_read8(COM1_BASE + SER_RBR, out) == LCOM_OK ? 0 : 1;
  }
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;

  lcom_irq_t irq = {0};
  if (lcom_irq_subscribe(COM1_IRQ, 0, &irq) != LCOM_OK) return 1;
  if (lcom_port_write8(COM1_BASE + SER_FCR, FCR_ENABLE_FIFO) != LCOM_OK) return 1;
  if (lcom_port_write8(COM1_BASE + SER_IER, IER_RDA) != LCOM_OK) return 1;
  if (lcom_port_write8(COM1_BASE + SER_THR, 'R') != LCOM_OK) return 1;

  char got[5] = {0};
  for (int i = 0; i < 4; i++) {
    uint8_t byte = 0;
    if (wait_rx(irq, &byte) != 0) return 1;
    got[i] = (char)byte;
  }

  lcom_printf("peer receiver got %s\n", got);
  lcom_irq_unsubscribe(&irq);
  lcom_exit();
  return 0;
}
