#include <lcom/ac97.h>
#include <lcom/lcom.h>

#include <stdint.h>
#include <stddef.h>

static int16_t sample_for(uint32_t frame, uint32_t sample_rate) {
  uint32_t period = sample_rate / 440u;
  if (period == 0) period = 1;
  return (frame % period) < (period / 2u) ? 9000 : -9000;
}

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;

  lcom_ac97_buffer_info_t info;
  if (lcom_ac97_get_buffer(&info) != LCOM_OK) return 1;

  int16_t *pcm = 0;
  if (lcom_phys_map(info.pcm_phys, info.pcm_bytes, (void **)&pcm) != LCOM_OK) return 1;

  uint32_t sample_rate = 48000;
  uint8_t channels = 2;
  uint32_t frames = sample_rate / 4u;
  size_t sample_count = (size_t)frames * channels;
  if (sample_count * sizeof(int16_t) > info.pcm_bytes) return 1;

  for (uint32_t frame = 0; frame < frames; frame++) {
    int16_t s = sample_for(frame, sample_rate);
    pcm[frame * channels + 0] = s;
    pcm[frame * channels + 1] = s;
  }

  size_t byte_count = sample_count * sizeof(int16_t);
  if (lcom_port_write16(AC97_NAM_BASE + AC97_PCM_FRONT_DAC_RATE, (uint16_t)sample_rate) != LCOM_OK) return 1;
  if (lcom_ac97_play(byte_count, sample_rate, channels) != LCOM_OK) return 1;
  lcom_printf("audio tone %u frames at %u Hz\n", frames, sample_rate);

  lcom_phys_unmap(pcm, info.pcm_bytes);
  lcom_exit();
  return 0;
}
