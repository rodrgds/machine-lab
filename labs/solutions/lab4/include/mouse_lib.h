#ifndef LAB4_MOUSE_LIB_H
#define LAB4_MOUSE_LIB_H

#include <lcom/i8042.h>

#include <stdint.h>

typedef struct {
  uint8_t bytes[3];
  int lb;
  int rb;
  int mb;
  int16_t dx;
  int16_t dy;
} mouse_packet_t;

int mouse_enable_data_reporting(void);
int mouse_disable_data_reporting(void);
int mouse_process_byte(uint8_t byte);
int mouse_get_packet(mouse_packet_t *packet);

#endif
