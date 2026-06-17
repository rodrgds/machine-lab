#include "lab_test.h"

#include "uart_lib.h"

static int wait_for_byte(lcom_irq_t irq, uint16_t base, uint8_t *byte) {
  for (int i = 0; i < 64; i++) {
    lcom_event_t ev = {0};
    if (lcom_event_wait(&ev) != LCOM_OK) return LCOM_ERR;
    if ((ev.irq_mask & irq.mask) == 0) continue;
    if (uart_read_byte(base, byte) == LCOM_OK) return LCOM_OK;
  }
  return LCOM_ERR;
}

static int check_divisor(uint16_t base, uint16_t expected) {
  uint8_t dll = 0;
  uint8_t dlm = 0;
  uint8_t lcr = 0;
  if (lcom_port_read8(base + SER_LCR, &lcr) != LCOM_OK) return 0;
  if (lcom_port_write8(base + SER_LCR, (uint8_t)(lcr | LCR_DLAB)) != LCOM_OK) return 0;
  if (lcom_port_read8(base + SER_DLL, &dll) != LCOM_OK) return 0;
  if (lcom_port_read8(base + SER_DLM, &dlm) != LCOM_OK) return 0;
  if (lcom_port_write8(base + SER_LCR, lcr) != LCOM_OK) return 0;
  return (uint16_t)(dll | (dlm << 8)) == expected;
}

int main(void) {
  int failures = 0;

  TEST_REQUIRE(lcom_init() == LCOM_OK);
  TEST_CHECK(uart_config(COM1_BASE, 9600, 0x03) == LCOM_OK);
  TEST_CHECK(check_divisor(COM1_BASE, 12));
  TEST_CHECK(uart_enable_fifo(COM1_BASE) == LCOM_OK);
  TEST_CHECK(uart_set_loopback(COM1_BASE, 1) == LCOM_OK);
  TEST_CHECK(uart_enable_rx_interrupt(COM1_BASE) == LCOM_OK);

  lcom_irq_t com1_irq = {0};
  TEST_REQUIRE(uart_subscribe(COM1_IRQ, &com1_irq) == LCOM_OK);
  TEST_CHECK(uart_send_byte(COM1_BASE, 'S') == LCOM_OK);
  uint8_t byte = 0;
  TEST_CHECK(wait_for_byte(com1_irq, COM1_BASE, &byte) == LCOM_OK);
  TEST_CHECK(byte == 'S');
  TEST_CHECK(uart_unsubscribe(&com1_irq) == LCOM_OK);
  TEST_CHECK(uart_set_loopback(COM1_BASE, 0) == LCOM_OK);

  lcom_irq_t com2_irq = {0};
  TEST_REQUIRE(uart_subscribe(COM2_IRQ, &com2_irq) == LCOM_OK);
  TEST_CHECK(uart_config(COM2_BASE, 19200, 0x03) == LCOM_OK);
  TEST_CHECK(check_divisor(COM2_BASE, 6));
  TEST_CHECK(uart_enable_fifo(COM2_BASE) == LCOM_OK);
  TEST_CHECK(uart_enable_rx_interrupt(COM2_BASE) == LCOM_OK);
  TEST_CHECK(uart_send_byte(COM1_BASE, '7') == LCOM_OK);
  TEST_CHECK(wait_for_byte(com2_irq, COM2_BASE, &byte) == LCOM_OK);
  TEST_CHECK(byte == '7');
  TEST_CHECK(uart_unsubscribe(&com2_irq) == LCOM_OK);

  lcom_printf("lab7 uart loopback and pair ok\n");
  lcom_exit();

  TEST_DONE("lab7");
}
