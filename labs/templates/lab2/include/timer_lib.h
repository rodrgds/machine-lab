#ifndef LAB2_TIMER_LIB_H
#define LAB2_TIMER_LIB_H

#include <lcom/i8254.h>
#include <lcom/lcom.h>

#include <stdint.h>

int timer_set_frequency(uint8_t timer, uint32_t freq);
int timer_get_conf(uint8_t timer, uint8_t *status);
int timer_subscribe(lcom_irq_t *irq);
int timer_unsubscribe(lcom_irq_t *irq);
void timer_ih(void);
uint32_t timer_ticks(void);

#endif
