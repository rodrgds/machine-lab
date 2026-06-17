#include "lab_test.h"

#include "video_lib.h"

static const char *const kSprite[] = {
    "4 4 3 1",
    ". c None",
    "r c #ff3030",
    "g c #30ff60",
    "r..g",
    ".rr.",
    ".gg.",
    "g..r",
};

int main(void) {
  int failures = 0;

  TEST_REQUIRE(lcom_init() == LCOM_OK);
  TEST_CHECK(video_set_mode(LCOM_VBE_MODE_800_600_24) == LCOM_OK);
  TEST_CHECK(video_map_framebuffer() == LCOM_OK);
  TEST_CHECK(video_fill_rect(10, 10, 32, 24, 0x2040A0u) == LCOM_OK);
  TEST_CHECK(video_draw_xpm(kSprite, 20, 18) == LCOM_OK);
  TEST_CHECK(video_draw_xpm(kSprite, -2, -2) == LCOM_OK);
  TEST_CHECK(video_present() == LCOM_OK);
  lcom_printf("lab5 frame rendered\n");
  lcom_exit();

  TEST_DONE("lab5");
}
