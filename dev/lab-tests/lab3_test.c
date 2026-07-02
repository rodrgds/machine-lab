#include "lab_test.h"

#include "keyboard_lib.h"

static int read_kbd_byte(lcom_irq_t irq, uint8_t *byte) {
  for (;;) {
    lcom_event_t ev = {0};
    if (lcom_event_wait(&ev) != LCOM_OK) return 1;
    if ((ev.irq_mask & irq.mask) == 0) continue;

    uint8_t status = 0;
    if (kbc_read_status(&status) != LCOM_OK) return 1;
    if ((status & KBC_ST_OBF) == 0 || (status & KBC_ST_AUX) != 0) continue;
    return kbc_read_output(byte) == LCOM_OK ? 0 : 1;
  }
}

int main(void) {
  int failures = 0;

  TEST_REQUIRE(lcom_init() == LCOM_OK);
  lcom_irq_t irq = {0};
  TEST_REQUIRE(lcom_irq_subscribe(KBC_IRQ, 0, &irq) == LCOM_OK);

  uint8_t byte = 0;
  uint8_t code[2] = {0};
  uint8_t size = 0;
  int make = 0;
  int saw_a = 0;
  int saw_esc_break = 0;

  while (!saw_esc_break) {
    TEST_REQUIRE(read_kbd_byte(irq, &byte) == 0);
    if (!kbd_process_byte(byte)) continue;
    TEST_REQUIRE(kbd_get_scancode(code, &size, &make) == LCOM_OK);
    if (size == 1 && code[0] == 0x1E && make) saw_a = 1;
    if (size == 1 && code[0] == ESC_BREAK && !make) saw_esc_break = 1;
  }

  TEST_CHECK(saw_a);
  TEST_CHECK(saw_esc_break);
  TEST_CHECK(lcom_irq_unsubscribe(&irq) == LCOM_OK);
  lcom_exit();

  TEST_DONE("lab3");
}
