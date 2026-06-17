#include <lcom/lcom.h>
#include <lcom/uart16550.h>

#include <stdint.h>

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;
  lcom_irq_t irq;
  if (lcom_irq_subscribe(COM1_IRQ, 0, &irq) != LCOM_OK) return 1;
  if (lcom_port_write8(COM1_BASE + SER_MCR, MCR_LOOP) != LCOM_OK) return 1;
  if (lcom_port_write8(COM1_BASE + SER_IER, IER_RDA) != LCOM_OK) return 1;
  if (lcom_port_write8(COM1_BASE + SER_THR, 'L') != LCOM_OK) return 1;

  lcom_event_t ev;
  if (lcom_event_wait(&ev) != LCOM_OK) return 1;
  if (ev.irq_mask & irq.mask) {
    uint8_t data = 0;
    if (lcom_port_read8(COM1_BASE + SER_RBR, &data) != LCOM_OK) return 1;
    lcom_printf("uart rx %c\n", data);
  }

  lcom_irq_unsubscribe(&irq);
  lcom_exit();
  return 0;
}
