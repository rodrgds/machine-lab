#ifndef LAB7_UART_LIB_H
#define LAB7_UART_LIB_H

#include <lcom/lcom.h>
#include <lcom/uart16550.h>

#include <stdint.h>

int uart_config(uint16_t base, uint32_t baud, uint8_t line_control);
int uart_enable_fifo(uint16_t base);
int uart_enable_rx_interrupt(uint16_t base);
int uart_set_loopback(uint16_t base, int enabled);
int uart_send_byte(uint16_t base, uint8_t byte);
int uart_read_byte(uint16_t base, uint8_t *byte);
int uart_subscribe(uint8_t irq, lcom_irq_t *out);
int uart_unsubscribe(lcom_irq_t *irq);

#endif
