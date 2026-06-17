#ifndef LAB6_AUDIO_LIB_H
#define LAB6_AUDIO_LIB_H

#include <lcom/ac97.h>

#include <stddef.h>

int audio_map_buffer(void);
int audio_fill_square_wave(uint32_t hz, uint32_t ms);
int audio_play(size_t byte_count);
int audio_stop(void);

#endif
