#include "audio_lib.h"

#include <stdint.h>

static lcom_ac97_buffer_info_t g_info;
static int16_t *g_pcm;
static size_t g_last_bytes;

int audio_map_buffer(void) {
  if (lcom_ac97_get_buffer(&g_info) != LCOM_OK) return LCOM_ERR;
  return lcom_phys_map(g_info.pcm_phys, g_info.pcm_bytes, (void **)&g_pcm);
}

int audio_fill_square_wave(uint32_t hz, uint32_t ms) {
  if (g_pcm == 0 || hz == 0) return LCOM_ERR;
  uint32_t frames = (g_info.sample_rate * ms) / 1000u;
  uint32_t period = g_info.sample_rate / hz;
  if (period == 0) period = 1;
  size_t samples = (size_t)frames * g_info.channels;
  if (samples * sizeof(int16_t) > g_info.pcm_bytes) return LCOM_ERR;
  for (uint32_t frame = 0; frame < frames; frame++) {
    int16_t value = (frame % period) < (period / 2u) ? 7000 : -7000;
    for (uint8_t ch = 0; ch < g_info.channels; ch++) {
      g_pcm[frame * g_info.channels + ch] = value;
    }
  }
  g_last_bytes = samples * sizeof(int16_t);
  return LCOM_OK;
}

int audio_play(size_t byte_count) {
  if (byte_count == 0) byte_count = g_last_bytes;
  return lcom_ac97_play(byte_count, g_info.sample_rate, g_info.channels);
}

int audio_stop(void) {
  return lcom_ac97_stop();
}
