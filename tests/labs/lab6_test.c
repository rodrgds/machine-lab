#include "lab_test.h"

#include "audio_lib.h"

int main(void) {
  int failures = 0;

  TEST_REQUIRE(lcom_init() == LCOM_OK);
  TEST_CHECK(audio_map_buffer() == LCOM_OK);
  TEST_CHECK(audio_fill_square_wave(440, 80) == LCOM_OK);
  TEST_CHECK(audio_play(0) == LCOM_OK);
  uint8_t cr = 0;
  uint8_t sr = 0;
  TEST_CHECK(lcom_port_read8(AC97_BM_BASE + AC97_PO_CR, &cr) == LCOM_OK);
  TEST_CHECK((cr & AC97_PO_CR_RUN) != 0);
  TEST_CHECK(lcom_port_read8(AC97_BM_BASE + AC97_PO_SR, &sr) == LCOM_OK);
  TEST_CHECK((sr & AC97_PO_SR_BCIS) != 0);
  TEST_CHECK(audio_play(0) == LCOM_OK);
  TEST_CHECK(lcom_port_read8(AC97_BM_BASE + AC97_PO_CR, &cr) == LCOM_OK);
  TEST_CHECK((cr & AC97_PO_CR_RUN) != 0);
  TEST_CHECK(audio_stop() == LCOM_OK);
  TEST_CHECK(lcom_port_read8(AC97_BM_BASE + AC97_PO_CR, &cr) == LCOM_OK);
  TEST_CHECK((cr & AC97_PO_CR_RUN) == 0);
  lcom_printf("lab6 audio rendered\n");
  lcom_exit();

  TEST_DONE("lab6");
}
