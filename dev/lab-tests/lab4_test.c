#include "lab_test.h"

#include "mouse_lib.h"

static int read_mouse_byte(lcom_irq_t irq, uint8_t *byte) {
  for (;;) {
    lcom_event_t ev = {0};
    if (lcom_event_wait(&ev) != LCOM_OK) return 1;
    if ((ev.irq_mask & irq.mask) == 0) continue;

    uint8_t status = 0;
    if (lcom_port_read8(KBC_ST_REG, &status) != LCOM_OK) return 1;
    if ((status & KBC_ST_OBF) == 0 || (status & KBC_ST_AUX) == 0) continue;
    return lcom_port_read8(KBC_OUT_BUF, byte) == LCOM_OK ? 0 : 1;
  }
}

int main(void) {
  int failures = 0;

  TEST_REQUIRE(lcom_init() == LCOM_OK);
  lcom_irq_t irq = {0};
  TEST_REQUIRE(lcom_irq_subscribe(MOUSE_IRQ, 0, &irq) == LCOM_OK);
  TEST_REQUIRE(mouse_enable_data_reporting() == LCOM_OK);

  uint8_t byte = 0;
  TEST_REQUIRE(read_mouse_byte(irq, &byte) == 0);
  TEST_REQUIRE(byte == MOUSE_ACK);

  mouse_packet_t packet = {0};
  for (;;) {
    TEST_REQUIRE(read_mouse_byte(irq, &byte) == 0);
    if (mouse_process_byte(byte)) {
      TEST_REQUIRE(mouse_get_packet(&packet) == LCOM_OK);
      break;
    }
  }

  TEST_CHECK(packet.lb == 1);
  TEST_CHECK(packet.rb == 0);
  TEST_CHECK(packet.mb == 0);
  TEST_CHECK(packet.dx == 5);
  TEST_CHECK(packet.dy == -2);
  TEST_CHECK(mouse_disable_data_reporting() == LCOM_OK);
  TEST_CHECK(lcom_irq_unsubscribe(&irq) == LCOM_OK);
  lcom_exit();

  TEST_DONE("lab4");
}
