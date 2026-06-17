#include "uart_lib.h"

#define UART_BASE_RATE 115200u

static int uart_read_reg(uint16_t base, uint8_t reg, uint8_t *value) {
  return lcom_port_read8((uint16_t)(base + reg), value);
}

static int uart_write_reg(uint16_t base, uint8_t reg, uint8_t value) {
  return lcom_port_write8((uint16_t)(base + reg), value);
}

int uart_config(uint16_t base, uint32_t baud, uint8_t line_control) {
  if (baud == 0) return LCOM_ERR;
  uint16_t divisor = (uint16_t)(UART_BASE_RATE / baud);
  if (divisor == 0) divisor = 1;

  if (uart_write_reg(base, SER_LCR, (uint8_t)(line_control | LCR_DLAB)) != LCOM_OK) {
    return LCOM_ERR;
  }
  if (uart_write_reg(base, SER_DLL, (uint8_t)(divisor & 0xFFu)) != LCOM_OK) {
    return LCOM_ERR;
  }
  if (uart_write_reg(base, SER_DLM, (uint8_t)(divisor >> 8)) != LCOM_OK) {
    return LCOM_ERR;
  }
  return uart_write_reg(base, SER_LCR, (uint8_t)(line_control & ~LCR_DLAB));
}

int uart_enable_fifo(uint16_t base) {
  return uart_write_reg(base, SER_FCR,
                        (uint8_t)(FCR_ENABLE_FIFO | FCR_CLEAR_RX | FCR_CLEAR_TX));
}

int uart_enable_rx_interrupt(uint16_t base) {
  return uart_write_reg(base, SER_IER, IER_RDA);
}

int uart_set_loopback(uint16_t base, int enabled) {
  uint8_t mcr = 0;
  if (uart_read_reg(base, SER_MCR, &mcr) != LCOM_OK) return LCOM_ERR;
  if (enabled) {
    mcr |= MCR_LOOP;
  } else {
    mcr &= (uint8_t)~MCR_LOOP;
  }
  return uart_write_reg(base, SER_MCR, mcr);
}

int uart_send_byte(uint16_t base, uint8_t byte) {
  uint8_t lsr = 0;
  if (uart_read_reg(base, SER_LSR, &lsr) != LCOM_OK) return LCOM_ERR;
  if ((lsr & LSR_THRE) == 0) return LCOM_ERR;
  return uart_write_reg(base, SER_THR, byte);
}

int uart_read_byte(uint16_t base, uint8_t *byte) {
  if (byte == 0) return LCOM_ERR;
  uint8_t lsr = 0;
  if (uart_read_reg(base, SER_LSR, &lsr) != LCOM_OK) return LCOM_ERR;
  if ((lsr & LSR_RX_RDY) == 0) return LCOM_ERR;
  return uart_read_reg(base, SER_RBR, byte);
}

int uart_subscribe(uint8_t irq, lcom_irq_t *out) {
  return lcom_irq_subscribe(irq, 0, out);
}

int uart_unsubscribe(lcom_irq_t *irq) {
  return lcom_irq_unsubscribe(irq);
}
