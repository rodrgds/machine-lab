#include <lcom/lcom.h>
#include <lcom/ac97.h>
#include "lcom_protocol.h"

#include <errno.h>
#include <fcntl.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

static int g_fd = -1;
static uint32_t g_next_request = 1;

static int write_all(int fd, const void *buf, size_t len) {
  const uint8_t *p = (const uint8_t *)buf;
  while (len > 0) {
    ssize_t n = write(fd, p, len);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (n == 0) return -1;
    p += (size_t)n;
    len -= (size_t)n;
  }
  return 0;
}

static int read_all(int fd, void *buf, size_t len) {
  uint8_t *p = (uint8_t *)buf;
  while (len > 0) {
    ssize_t n = read(fd, p, len);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (n == 0) return -1;
    p += (size_t)n;
    len -= (size_t)n;
  }
  return 0;
}

static int send_msg(uint16_t type, uint32_t request_id, const void *payload, uint32_t size) {
  if (g_fd < 0) return -1;
  if (size > LCOM_MAX_PAYLOAD) return -1;

  lcom_msg_header_t hdr;
  hdr.type = type;
  hdr.flags = 0;
  hdr.size = size;
  hdr.request_id = request_id;

  if (write_all(g_fd, &hdr, sizeof(hdr)) != 0) return -1;
  if (size != 0 && write_all(g_fd, payload, size) != 0) return -1;
  return 0;
}

static int recv_msg(lcom_msg_header_t *hdr, void *payload, uint32_t capacity) {
  if (g_fd < 0) return -1;
  if (read_all(g_fd, hdr, sizeof(*hdr)) != 0) return -1;
  if (hdr->size > capacity || hdr->size > LCOM_MAX_PAYLOAD) return -1;
  if (hdr->size != 0 && read_all(g_fd, payload, hdr->size) != 0) return -1;
  return 0;
}

static int request_reply(uint16_t req_type,
                         const void *req,
                         uint32_t req_size,
                         uint16_t reply_type,
                         void *reply,
                         uint32_t reply_capacity,
                         uint32_t *reply_size) {
  uint32_t request_id = g_next_request++;
  if (send_msg(req_type, request_id, req, req_size) != 0) return -1;

  for (;;) {
    uint8_t buf[LCOM_MAX_PAYLOAD];
    lcom_msg_header_t hdr;
    if (recv_msg(&hdr, buf, sizeof(buf)) != 0) return -1;
    if (hdr.request_id != request_id) continue;
    if (hdr.type != reply_type) return -1;
    if (hdr.size != reply_capacity) return -1;
    memcpy(reply, buf, hdr.size);
    if (reply_size != NULL) *reply_size = hdr.size;
    return 0;
  }
}

int lcom_init(void) {
  if (g_fd >= 0) return LCOM_OK;

  const char *fd_env = getenv("MACHINE_LAB_RUN_FD");
  if (fd_env == NULL || fd_env[0] == '\0') {
    fd_env = getenv("LCOM_RUN_FD");
  }
  if (fd_env == NULL || fd_env[0] == '\0') {
    return LCOM_ERR;
  }

  char *end = NULL;
  long fd = strtol(fd_env, &end, 10);
  if (end == fd_env || fd < 0) return LCOM_ERR;

  g_fd = (int)fd;

  lcom_hello_t hello;
  hello.version = LCOM_PROTOCOL_VERSION;
  hello.pid = (uint32_t)getpid();

  lcom_hello_reply_t reply;
  if (request_reply(LCOM_MSG_HELLO, &hello, sizeof(hello), LCOM_MSG_HELLO_REPLY,
                    &reply, sizeof(reply), NULL) != 0) {
    g_fd = -1;
    return LCOM_ERR;
  }

  return reply.status == 0 && reply.version == LCOM_PROTOCOL_VERSION ? LCOM_OK : LCOM_ERR;
}

void lcom_exit(void) {
  if (g_fd >= 0) {
    send_msg(LCOM_MSG_EXIT, g_next_request++, NULL, 0);
    close(g_fd);
    g_fd = -1;
  }
}

int lcom_printf(const char *fmt, ...) {
  char stack_buf[1024];
  va_list ap;
  va_start(ap, fmt);
  int n = vsnprintf(stack_buf, sizeof(stack_buf), fmt, ap);
  va_end(ap);
  if (n < 0) return LCOM_ERR;

  if (g_fd < 0) {
    fputs(stack_buf, stdout);
    return LCOM_OK;
  }

  size_t len = (size_t)n;
  if (len >= sizeof(stack_buf)) len = sizeof(stack_buf) - 1;
  if (send_msg(LCOM_MSG_CONSOLE_WRITE, 0, stack_buf, (uint32_t)len) != 0) {
    return LCOM_ERR;
  }
  return LCOM_OK;
}

int lcom_port_read8(uint16_t port, uint8_t *value) {
  if (value == NULL) return LCOM_ERR;
  lcom_port_read8_t req;
  req.port = port;
  req.reserved = 0;

  lcom_port_read8_reply_t reply;
  if (request_reply(LCOM_MSG_PORT_READ8, &req, sizeof(req), LCOM_MSG_PORT_READ8_REPLY,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  if (reply.status != 0) return LCOM_ERR;
  *value = reply.value;
  return LCOM_OK;
}

int lcom_port_write8(uint16_t port, uint8_t value) {
  lcom_port_write8_t req;
  req.port = port;
  req.value = value;
  req.reserved = 0;

  lcom_status_reply_t reply;
  if (request_reply(LCOM_MSG_PORT_WRITE8, &req, sizeof(req), LCOM_MSG_STATUS,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  return reply.status == 0 ? LCOM_OK : LCOM_ERR;
}

int lcom_port_read16(uint16_t port, uint16_t *value) {
  if (value == NULL) return LCOM_ERR;
  uint8_t lo = 0;
  uint8_t hi = 0;
  if (lcom_port_read8(port, &lo) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_read8((uint16_t)(port + 1u), &hi) != LCOM_OK) return LCOM_ERR;
  *value = (uint16_t)(lo | ((uint16_t)hi << 8));
  return LCOM_OK;
}

int lcom_port_write16(uint16_t port, uint16_t value) {
  if (lcom_port_write8(port, (uint8_t)(value & 0xFFu)) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_write8((uint16_t)(port + 1u), (uint8_t)(value >> 8)) != LCOM_OK) return LCOM_ERR;
  return LCOM_OK;
}

int lcom_port_read32(uint16_t port, uint32_t *value) {
  if (value == NULL) return LCOM_ERR;
  uint16_t lo = 0;
  uint16_t hi = 0;
  if (lcom_port_read16(port, &lo) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_read16((uint16_t)(port + 2u), &hi) != LCOM_OK) return LCOM_ERR;
  *value = (uint32_t)lo | ((uint32_t)hi << 16);
  return LCOM_OK;
}

int lcom_port_write32(uint16_t port, uint32_t value) {
  if (lcom_port_write16(port, (uint16_t)(value & 0xFFFFu)) != LCOM_OK) return LCOM_ERR;
  if (lcom_port_write16((uint16_t)(port + 2u), (uint16_t)(value >> 16)) != LCOM_OK) return LCOM_ERR;
  return LCOM_OK;
}

int lcom_irq_subscribe(uint8_t irq, uint32_t flags, lcom_irq_t *out) {
  if (out == NULL) return LCOM_ERR;
  lcom_irq_subscribe_t req;
  req.irq = irq;
  memset(req.reserved, 0, sizeof(req.reserved));
  req.flags = flags;

  lcom_irq_subscribe_reply_t reply;
  if (request_reply(LCOM_MSG_IRQ_SUBSCRIBE, &req, sizeof(req),
                    LCOM_MSG_IRQ_SUBSCRIBE_REPLY, &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  if (reply.status != 0) return LCOM_ERR;
  out->irq = reply.irq;
  out->bit_no = reply.bit_no;
  out->mask = reply.mask;
  return LCOM_OK;
}

int lcom_irq_unsubscribe(lcom_irq_t *irq) {
  if (irq == NULL) return LCOM_ERR;
  lcom_irq_unsubscribe_t req;
  req.irq = irq->irq;
  memset(req.reserved, 0, sizeof(req.reserved));

  lcom_status_reply_t reply;
  if (request_reply(LCOM_MSG_IRQ_UNSUBSCRIBE, &req, sizeof(req), LCOM_MSG_STATUS,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  if (reply.status != 0) return LCOM_ERR;
  irq->mask = 0;
  return LCOM_OK;
}

int lcom_event_wait(lcom_event_t *event) {
  if (event == NULL) return LCOM_ERR;

  lcom_event_reply_t reply;
  if (request_reply(LCOM_MSG_EVENT_WAIT, NULL, 0, LCOM_MSG_EVENT_REPLY,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  if (reply.status != 0) return LCOM_ERR;
  event->irq_mask = reply.irq_mask;
  event->tick = reply.tick;
  return LCOM_OK;
}

int lcom_phys_map(uintptr_t phys, size_t len, void **out) {
  if (out == NULL || len == 0) return LCOM_ERR;

  lcom_phys_map_t req;
  req.phys = (uint64_t)phys;
  req.length = (uint64_t)len;

  lcom_phys_map_reply_t reply;
  if (request_reply(LCOM_MSG_PHYS_MAP, &req, sizeof(req), LCOM_MSG_PHYS_MAP_REPLY,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  if (reply.status != 0) return LCOM_ERR;

  int fd = shm_open(reply.shm_name, O_RDWR, 0600);
  if (fd < 0) return LCOM_ERR;

  void *mapped = mmap(NULL, (size_t)reply.length, PROT_READ | PROT_WRITE, MAP_SHARED,
                      fd, (off_t)reply.offset);
  close(fd);
  if (mapped == MAP_FAILED) return LCOM_ERR;

  *out = mapped;
  return LCOM_OK;
}

int lcom_phys_unmap(void *ptr, size_t len) {
  if (ptr == NULL || len == 0) return LCOM_ERR;
  return munmap(ptr, len) == 0 ? LCOM_OK : LCOM_ERR;
}

int lcom_vbe_get_mode_info(uint16_t mode, lcom_vbe_mode_info_t *out) {
  if (out == NULL) return LCOM_ERR;

  lcom_vbe_mode_request_t req;
  req.mode = mode;
  req.reserved = 0;

  lcom_vbe_mode_info_wire_t reply;
  if (request_reply(LCOM_MSG_VBE_GET_MODE_INFO, &req, sizeof(req),
                    LCOM_MSG_VBE_MODE_INFO_REPLY, &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  if (reply.status != 0) return LCOM_ERR;

  out->mode = reply.mode;
  out->width = reply.width;
  out->height = reply.height;
  out->bpp = reply.bpp;
  out->bytes_per_pixel = reply.bytes_per_pixel;
  out->pitch = reply.pitch;
  out->framebuffer_phys = (uintptr_t)reply.framebuffer_phys;
  out->framebuffer_size = (size_t)reply.framebuffer_size;
  return LCOM_OK;
}

int lcom_vbe_set_mode(uint16_t mode) {
  lcom_vbe_mode_request_t req;
  req.mode = mode;
  req.reserved = 0;

  lcom_status_reply_t reply;
  if (request_reply(LCOM_MSG_VBE_SET_MODE, &req, sizeof(req), LCOM_MSG_STATUS,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  return reply.status == 0 ? LCOM_OK : LCOM_ERR;
}

int lcom_vbe_present(void) {
  lcom_status_reply_t reply;
  if (request_reply(LCOM_MSG_VBE_PRESENT, NULL, 0, LCOM_MSG_STATUS,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  return reply.status == 0 ? LCOM_OK : LCOM_ERR;
}

int lcom_ac97_get_buffer(lcom_ac97_buffer_info_t *out) {
  if (out == NULL) return LCOM_ERR;
  lcom_ac97_buffer_wire_t reply;
  if (request_reply(LCOM_MSG_AC97_GET_BUFFER, NULL, 0, LCOM_MSG_AC97_BUFFER_REPLY,
                    &reply, sizeof(reply), NULL) != 0) {
    return LCOM_ERR;
  }
  if (reply.status != 0) return LCOM_ERR;
  out->pcm_phys = (uintptr_t)reply.pcm_phys;
  out->pcm_bytes = (size_t)reply.pcm_bytes;
  out->sample_rate = reply.sample_rate;
  out->channels = reply.channels;
  out->bits_per_sample = reply.bits_per_sample;
  return LCOM_OK;
}
