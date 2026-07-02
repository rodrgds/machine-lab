#include "lab_test.h"

#include "timer_lib.h"

int main(void) {
  int failures = 0;

  TEST_REQUIRE(lcom_init() == LCOM_OK);

  uint8_t status = 0;
  TEST_CHECK(timer_get_conf(0, &status) == LCOM_OK);
  TEST_CHECK((status & TIMER_LSB_MSB) == TIMER_LSB_MSB);

  TEST_CHECK(timer_set_frequency(0, 60) == LCOM_OK);
  TEST_CHECK(timer_set_frequency(0, 0) == LCOM_ERR);
  TEST_CHECK(timer_get_conf(0, &status) == LCOM_OK);
  TEST_CHECK((status & TIMER_LSB_MSB) == TIMER_LSB_MSB);

  lcom_irq_t irq = {0};
  TEST_REQUIRE(timer_subscribe(&irq) == LCOM_OK);
  while (timer_ticks() < 4) {
    lcom_event_t ev = {0};
    TEST_REQUIRE(lcom_event_wait(&ev) == LCOM_OK);
    if ((ev.irq_mask & irq.mask) != 0) timer_ih();
  }
  TEST_CHECK(timer_ticks() == 4);
  TEST_CHECK(timer_unsubscribe(&irq) == LCOM_OK);

  lcom_exit();
  TEST_DONE("lab2");
}
