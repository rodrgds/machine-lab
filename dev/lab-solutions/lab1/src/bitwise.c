#include "bitwise.h"

#include <stdarg.h>

#ifndef BIT
#define BIT(n) (1u << (n))
#endif

uint8_t bit_clear(uint8_t value, uint8_t bit) {
  return value & (uint8_t)~BIT(bit);
}

uint8_t bit_set(uint8_t value, uint8_t bit) {
  return value | (uint8_t)BIT(bit);
}

int bit_is_set(uint8_t value, uint8_t bit) {
  return (value & BIT(bit)) != 0;
}

uint8_t bit_lsb(uint16_t value) {
  return (uint8_t)(value & 0xFFu);
}

uint8_t bit_msb(uint16_t value) {
  return (uint8_t)(value >> 8);
}

uint8_t bit_mask(unsigned first_bit, ...) {
  uint8_t result = 0;
  va_list ap;
  va_start(ap, first_bit);
  for (unsigned bit = first_bit; bit != BIT_MASK_END; bit = (unsigned)va_arg(ap, int)) {
    result |= (uint8_t)BIT(bit);
  }
  va_end(ap);
  return result;
}
