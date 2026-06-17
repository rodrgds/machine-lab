#include "sound.h"

#include <lcom/ac97.h>

#include <stdint.h>

static lcom_ac97_buffer_info_t g_info;
static int16_t *g_pcm;
static int g_ready;

void sound_init(void) {
  if (lcom_ac97_get_buffer(&g_info) != LCOM_OK) return;
  if (lcom_phys_map(g_info.pcm_phys, g_info.pcm_bytes, (void **)&g_pcm) != LCOM_OK) return;
  g_ready = 1;
}

void sound_flap(void) {
  if (!g_ready) return;
  uint32_t frames = g_info.sample_rate / 18u;
  uint32_t period = g_info.sample_rate / 660u;
  if (period == 0) period = 1;
  for (uint32_t frame = 0; frame < frames; frame++) {
    int16_t sample = (frame % period) < (period / 2u) ? 6000 : -6000;
    for (uint8_t ch = 0; ch < g_info.channels; ch++) {
      g_pcm[frame * g_info.channels + ch] = sample;
    }
  }
  lcom_ac97_play((size_t)frames * g_info.channels * sizeof(int16_t),
                 g_info.sample_rate,
                 g_info.channels);
}
