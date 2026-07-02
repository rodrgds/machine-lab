#ifndef LAB1_BITWISE_H
#define LAB1_BITWISE_H

#include <stdint.h>

#define BIT_MASK_END 0xFFu

uint8_t bit_clear(uint8_t value, uint8_t bit);
uint8_t bit_set(uint8_t value, uint8_t bit);
int bit_is_set(uint8_t value, uint8_t bit);
uint8_t bit_lsb(uint16_t value);
uint8_t bit_msb(uint16_t value);
uint8_t bit_mask(unsigned first_bit, ...);

#endif
