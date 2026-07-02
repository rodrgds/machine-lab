#ifndef LAB3_KEYBOARD_LIB_H
#define LAB3_KEYBOARD_LIB_H

#include <lcom/i8042.h>

#include <stdint.h>

int kbc_read_status(uint8_t *status);
int kbc_read_output(uint8_t *byte);
int kbc_write_command(uint8_t command);
int kbd_process_byte(uint8_t byte);
int kbd_get_scancode(uint8_t bytes[2], uint8_t *size, int *make);

#endif
