#include "lab_test.h"

#include "audio_lib.h"

int main(void) {
  int failures = 0;

  TEST_REQUIRE(lcom_init() == LCOM_OK);
  TEST_CHECK(audio_map_buffer() == LCOM_OK);
  TEST_CHECK(audio_fill_square_wave(440, 80) == LCOM_OK);
  TEST_CHECK(audio_play(0) == LCOM_OK);
  TEST_CHECK(audio_stop() == LCOM_OK);
  lcom_printf("lab6 audio rendered\n");
  lcom_exit();

  TEST_DONE("lab6");
}
