#include "timer_lib.h"

static uint32_t g_ticks;

int timer_set_frequency(uint8_t timer, uint32_t freq) {
  if (timer > 2 || freq == 0 || freq > TIMER_FREQ) return LCOM_ERR;
  uint32_t divisor = TIMER_FREQ / freq;
  if (divisor == 0 || divisor > 65536u) return LCOM_ERR;

  uint8_t status = 0;
  if (timer_get_conf(timer, &status) != LCOM_OK) return LCOM_ERR;

  uint8_t control = (uint8_t)((timer << 6) | TIMER_LSB_MSB | (status & 0x0Fu));
  uint16_t encoded = (uint16_t)(divisor == 65536u ? 0u : divisor);
  if (lcom_port_write8(TIMER_CTRL, control) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_write8((uint16_t)(TIMER_0 + timer), (uint8_t)(encoded & 0xFFu)) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_write8((uint16_t)(TIMER_0 + timer), (uint8_t)(encoded >> 8)) != LCOM_OK) return LCOM_ERR;
  return LCOM_OK;
}

int timer_get_conf(uint8_t timer, uint8_t *status) {
  if (status == 0 || timer > 2) return LCOM_ERR;
  uint8_t rb = TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(timer);
  if (lcom_port_write8(TIMER_CTRL, rb) != LCOM_OK) return LCOM_ERR;
  return lcom_port_read8((uint16_t)(TIMER_0 + timer), status);
}

int timer_subscribe(lcom_irq_t *irq) {
  return lcom_irq_subscribe(TIMER0_IRQ, 0, irq);
}

int timer_unsubscribe(lcom_irq_t *irq) {
  return lcom_irq_unsubscribe(irq);
}

void timer_ih(void) {
  g_ticks++;
}

uint32_t timer_ticks(void) {
  return g_ticks;
}
