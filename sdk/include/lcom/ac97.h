#ifndef MACHINE_LAB_AC97_H
#define MACHINE_LAB_AC97_H

#include <lcom/lcom.h>

#define AC97_NAM_BASE 0xC000u
#define AC97_BM_BASE 0xC100u

#define AC97_RESET 0x00u
#define AC97_MASTER_VOLUME 0x02u
#define AC97_PCM_OUT_VOLUME 0x18u
#define AC97_EXT_AUDIO_ID 0x28u
#define AC97_EXT_AUDIO_CTRL 0x2Au
#define AC97_PCM_FRONT_DAC_RATE 0x2Cu

#define AC97_PO_BDBAR 0x10u
#define AC97_PO_CIV 0x14u
#define AC97_PO_LVI 0x15u
#define AC97_PO_SR 0x16u
#define AC97_PO_PICB 0x18u
#define AC97_PO_PIV 0x1Au
#define AC97_PO_CR 0x1Bu

#define AC97_PO_CR_RUN BIT(0)
#define AC97_PO_CR_RESET BIT(1)
#define AC97_PO_SR_DCH BIT(0)
#define AC97_PO_SR_BCIS BIT(3)

#define AC97_DEFAULT_SAMPLE_RATE 48000u
#define AC97_DEFAULT_CHANNELS 2u
#define AC97_BITS_PER_SAMPLE 16u

typedef struct {
  uintptr_t pcm_phys;
  size_t pcm_bytes;
  uint32_t sample_rate;
  uint8_t channels;
  uint8_t bits_per_sample;
} lcom_ac97_buffer_info_t;

int lcom_ac97_get_buffer(lcom_ac97_buffer_info_t *out);

#endif
