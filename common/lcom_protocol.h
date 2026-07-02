#ifndef MACHINE_LAB_PROTOCOL_H
#define MACHINE_LAB_PROTOCOL_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define LCOM_PROTOCOL_VERSION 1u
#define LCOM_MAX_PAYLOAD 4096u
#define LCOM_MAX_SHM_NAME 128u

typedef enum {
  LCOM_MSG_HELLO = 1,
  LCOM_MSG_HELLO_REPLY = 2,
  LCOM_MSG_EXIT = 3,
  LCOM_MSG_STATUS = 4,
  LCOM_MSG_CONSOLE_WRITE = 5,
  LCOM_MSG_PORT_READ8 = 10,
  LCOM_MSG_PORT_READ8_REPLY = 11,
  LCOM_MSG_PORT_WRITE8 = 12,
  LCOM_MSG_IRQ_SUBSCRIBE = 20,
  LCOM_MSG_IRQ_SUBSCRIBE_REPLY = 21,
  LCOM_MSG_IRQ_UNSUBSCRIBE = 22,
  LCOM_MSG_EVENT_WAIT = 30,
  LCOM_MSG_EVENT_REPLY = 31,
  LCOM_MSG_PHYS_MAP = 40,
  LCOM_MSG_PHYS_MAP_REPLY = 41,
  LCOM_MSG_PHYS_UNMAP = 42,
  LCOM_MSG_VBE_GET_MODE_INFO = 50,
  LCOM_MSG_VBE_MODE_INFO_REPLY = 51,
  LCOM_MSG_VBE_SET_MODE = 52,
  LCOM_MSG_VBE_PRESENT = 53,
  LCOM_MSG_AC97_GET_BUFFER = 60,
  LCOM_MSG_AC97_BUFFER_REPLY = 61
} lcom_msg_type_t;

typedef struct {
  uint16_t type;
  uint16_t flags;
  uint32_t size;
  uint32_t request_id;
} lcom_msg_header_t;

typedef struct {
  uint32_t version;
  uint32_t pid;
} lcom_hello_t;

typedef struct {
  int32_t status;
  uint32_t version;
} lcom_hello_reply_t;

typedef struct {
  int32_t status;
} lcom_status_reply_t;

typedef struct {
  uint16_t port;
  uint16_t reserved;
} lcom_port_read8_t;

typedef struct {
  int32_t status;
  uint8_t value;
  uint8_t reserved[3];
} lcom_port_read8_reply_t;

typedef struct {
  uint16_t port;
  uint8_t value;
  uint8_t reserved;
} lcom_port_write8_t;

typedef struct {
  uint8_t irq;
  uint8_t reserved[3];
  uint32_t flags;
} lcom_irq_subscribe_t;

typedef struct {
  int32_t status;
  uint8_t irq;
  uint8_t bit_no;
  uint8_t reserved[2];
  uint32_t mask;
} lcom_irq_subscribe_reply_t;

typedef struct {
  uint8_t irq;
  uint8_t reserved[3];
} lcom_irq_unsubscribe_t;

typedef struct {
  int32_t status;
  uint32_t irq_mask;
  uint64_t tick;
} lcom_event_reply_t;

typedef struct {
  uint64_t phys;
  uint64_t length;
} lcom_phys_map_t;

typedef struct {
  int32_t status;
  char shm_name[LCOM_MAX_SHM_NAME];
  uint64_t offset;
  uint64_t length;
} lcom_phys_map_reply_t;

typedef struct {
  uint16_t mode;
  uint16_t reserved;
} lcom_vbe_mode_request_t;

typedef struct {
  int32_t status;
  uint16_t mode;
  uint16_t width;
  uint16_t height;
  uint8_t bpp;
  uint8_t bytes_per_pixel;
  uint16_t reserved;
  uint32_t pitch;
  uint64_t framebuffer_phys;
  uint64_t framebuffer_size;
} lcom_vbe_mode_info_wire_t;

typedef struct {
  int32_t status;
  uint64_t pcm_phys;
  uint64_t pcm_bytes;
  uint32_t sample_rate;
  uint8_t channels;
  uint8_t bits_per_sample;
  uint8_t reserved[2];
} lcom_ac97_buffer_wire_t;

#ifdef __cplusplus
}
#endif

#endif
