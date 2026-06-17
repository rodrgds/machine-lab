#include <lcom/i8254.h>
#include <lcom/lcom.h>

#include <stdint.h>
#include <stdlib.h>

int main(int argc, char **argv) {
  uint32_t ticks = 3;
  if (argc > 1) ticks = (uint32_t)strtoul(argv[1], 0, 10);
  if (lcom_init() != LCOM_OK) return 1;

  lcom_irq_t irq;
  if (lcom_irq_subscribe(TIMER0_IRQ, 0, &irq) != LCOM_OK) return 1;

  uint32_t seen = 0;
  while (seen < ticks) {
    lcom_event_t ev;
    if (lcom_event_wait(&ev) != LCOM_OK) return 1;
    if (ev.irq_mask & irq.mask) {
      seen++;
      lcom_printf("timer tick %u at virtual tick %llu\n", seen, (unsigned long long)ev.tick);
    }
  }

  lcom_irq_unsubscribe(&irq);
  lcom_exit();
  return 0;
}
