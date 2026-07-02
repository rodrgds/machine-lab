#ifndef MACHINE_LAB_UART16550_H
#define MACHINE_LAB_UART16550_H

#include <lcom/lcom.h>

#define COM1_BASE 0x3F8u
#define COM2_BASE 0x2F8u

#define SER_RBR 0u
#define SER_THR 0u
#define SER_IER 1u
#define SER_DLL 0u
#define SER_DLM 1u
#define SER_IIR 2u
#define SER_FCR 2u
#define SER_LCR 3u
#define SER_MCR 4u
#define SER_LSR 5u
#define SER_MSR 6u
#define SER_SR 7u

#define LCR_DLAB BIT(7)

#define FCR_ENABLE_FIFO BIT(0)
#define FCR_CLEAR_RX BIT(1)
#define FCR_CLEAR_TX BIT(2)

#define MCR_DTR BIT(0)
#define MCR_RTS BIT(1)
#define MCR_OUT1 BIT(2)
#define MCR_OUT2 BIT(3)
#define MCR_LOOP BIT(4)

#define LSR_RX_RDY BIT(0)
#define LSR_OE BIT(1)
#define LSR_PE BIT(2)
#define LSR_FE BIT(3)
#define LSR_BI BIT(4)
#define LSR_THRE BIT(5)
#define LSR_TEMT BIT(6)
#define LSR_FIFO_ERR BIT(7)

#define IER_RDA BIT(0)
#define IER_THRE BIT(1)
#define IER_RLS BIT(2)
#define IER_MODEM BIT(3)

#define IIR_NO_INT BIT(0)
#define IIR_RDA 0x04u
#define IIR_THRE 0x02u

#define COM1_IRQ 4u
#define COM2_IRQ 3u

#endif
