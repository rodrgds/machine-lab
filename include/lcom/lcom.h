#ifndef LCOM_NG_LCOM_H
#define LCOM_NG_LCOM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef BIT
#define BIT(n) (1u << (n))
#endif

#ifndef OK
#define OK 0
#endif

typedef enum {
  LCOM_OK = 0,
  LCOM_ERR = 1,
  LCOM_TIMEOUT = 2,
  LCOM_NO_DATA = 3
} lcom_result_t;

typedef struct {
  uint8_t irq;
  uint8_t bit_no;
  uint32_t mask;
} lcom_irq_t;

typedef struct {
  uint32_t irq_mask;
  uint64_t tick;
} lcom_event_t;

typedef struct {
  uint16_t mode;
  uint16_t width;
  uint16_t height;
  uint8_t bpp;
  uint8_t bytes_per_pixel;
  uint32_t pitch;
  uintptr_t framebuffer_phys;
  size_t framebuffer_size;
} lcom_vbe_mode_info_t;

int lcom_init(void);
void lcom_exit(void);

int lcom_printf(const char *fmt, ...);

int lcom_port_read8(uint16_t port, uint8_t *value);
int lcom_port_write8(uint16_t port, uint8_t value);
int lcom_port_read16(uint16_t port, uint16_t *value);
int lcom_port_write16(uint16_t port, uint16_t value);
int lcom_port_read32(uint16_t port, uint32_t *value);
int lcom_port_write32(uint16_t port, uint32_t value);

int lcom_irq_subscribe(uint8_t irq, uint32_t flags, lcom_irq_t *out);
int lcom_irq_unsubscribe(lcom_irq_t *irq);
int lcom_event_wait(lcom_event_t *event);

int lcom_phys_map(uintptr_t phys, size_t len, void **out);
int lcom_phys_unmap(void *ptr, size_t len);

int lcom_vbe_get_mode_info(uint16_t mode, lcom_vbe_mode_info_t *out);
int lcom_vbe_set_mode(uint16_t mode);
int lcom_vbe_present(void);

#ifdef __cplusplus
}
#endif

#endif
