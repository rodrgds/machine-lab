#include "lab_test.h"

#include "bitwise.h"
#include "rtc_lib.h"

#include <lcom/lcom.h>

int main(void) {
  int failures = 0;

  TEST_CHECK(bit_clear(0xFFu, 3) == 0xF7u);
  TEST_CHECK(bit_set(0x00u, 5) == 0x20u);
  TEST_CHECK(bit_is_set(0x20u, 5));
  TEST_CHECK(!bit_is_set(0x20u, 4));
  TEST_CHECK(bit_lsb(0xABCDu) == 0xCDu);
  TEST_CHECK(bit_msb(0xABCDu) == 0xABu);
  TEST_CHECK(bit_mask(0, 2, 7, BIT_MASK_END) == 0x85u);

  TEST_REQUIRE(lcom_init() == LCOM_OK);
  lcom_rtc_date_t date = {0};
  lcom_rtc_time_t time = {0};
  TEST_CHECK(rtc_read_date(&date) == LCOM_OK);
  TEST_CHECK(rtc_read_time(&time) == LCOM_OK);
  TEST_CHECK(date.year == 26);
  TEST_CHECK(date.month == 6);
  TEST_CHECK(date.day == 16);
  TEST_CHECK(time.hour == 12);
  TEST_CHECK(time.minute == 34);
  TEST_CHECK(time.second == 56);
  lcom_exit();

  TEST_DONE("lab1");
}
