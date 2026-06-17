#include "sound.h"

#include <lcom/ac97.h>

#include <stdint.h>

static lcom_ac97_buffer_info_t g_info;
static int16_t *g_pcm;
static int g_ready;

static int ac97_start_pcm(uint32_t sample_rate, size_t byte_count) {
  if (byte_count == 0 || byte_count > 0xFFFFu) return LCOM_ERR;
  if (lcom_port_write16(AC97_NAM_BASE + AC97_PCM_FRONT_DAC_RATE,
                        (uint16_t)sample_rate) != LCOM_OK) {
    return LCOM_ERR;
  }
  if (lcom_port_write16(AC97_BM_BASE + AC97_PO_PICB,
                        (uint16_t)byte_count) != LCOM_OK) {
    return LCOM_ERR;
  }
  return lcom_port_write8(AC97_BM_BASE + AC97_PO_CR, AC97_PO_CR_RUN);
}

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
  ac97_start_pcm(g_info.sample_rate,
                 (size_t)frames * g_info.channels * sizeof(int16_t));
}
