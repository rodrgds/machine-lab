#ifndef MACHINE_LAB_I8254_H
#define MACHINE_LAB_I8254_H

#include <lcom/lcom.h>

#define TIMER_FREQ 1193182u
#define TIMER0_IRQ 0u

#define TIMER_0 0x40u
#define TIMER_1 0x41u
#define TIMER_2 0x42u
#define TIMER_CTRL 0x43u

#define SPEAKER_CTRL 0x61u

#define TIMER_SEL0 0x00u
#define TIMER_SEL1 BIT(6)
#define TIMER_SEL2 BIT(7)
#define TIMER_RB_CMD (BIT(7) | BIT(6))

#define TIMER_LSB BIT(4)
#define TIMER_MSB BIT(5)
#define TIMER_LSB_MSB (TIMER_LSB | TIMER_MSB)

#define TIMER_SQR_WAVE (BIT(2) | BIT(1))
#define TIMER_RATE_GEN BIT(2)

#define TIMER_BCD 0x01u
#define TIMER_BIN 0x00u

#define TIMER_RB_COUNT_ BIT(5)
#define TIMER_RB_STATUS_ BIT(4)
#define TIMER_RB_SEL(n) BIT((n) + 1)

#endif
