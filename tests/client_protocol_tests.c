#include <lcom/lcom.h>
#include "lcom_protocol.h"

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>

static int read_all(int fd, void *buffer, size_t length) {
  uint8_t *cursor = buffer;
  while (length > 0) {
    ssize_t count = read(fd, cursor, length);
    if (count <= 0) return -1;
    cursor += (size_t)count;
    length -= (size_t)count;
  }
  return 0;
}

static int write_all(int fd, const void *buffer, size_t length) {
  const uint8_t *cursor = buffer;
  while (length > 0) {
    ssize_t count = write(fd, cursor, length);
    if (count <= 0) return -1;
    cursor += (size_t)count;
    length -= (size_t)count;
  }
  return 0;
}

static int rejects_hello_reply(uint32_t reply_size, uint32_t version) {
  int sockets[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sockets) != 0) return 0;

  pid_t child = fork();
  if (child < 0) return 0;
  if (child == 0) {
    close(sockets[0]);
    char fd_text[32];
    snprintf(fd_text, sizeof(fd_text), "%d", sockets[1]);
    setenv("MACHINE_LAB_RUN_FD", fd_text, 1);
    _exit(lcom_init() == LCOM_ERR ? 0 : 1);
  }

  close(sockets[1]);
  lcom_msg_header_t request_header = {0};
  lcom_hello_t request = {0};
  int ok = read_all(sockets[0], &request_header, sizeof(request_header)) == 0 &&
           request_header.type == LCOM_MSG_HELLO &&
           request_header.size == sizeof(request) &&
           read_all(sockets[0], &request, sizeof(request)) == 0;

  lcom_hello_reply_t reply = {.status = 0, .version = version};
  lcom_msg_header_t reply_header = {
      .type = LCOM_MSG_HELLO_REPLY,
      .flags = 0,
      .size = reply_size,
      .request_id = request_header.request_id,
  };
  ok = ok && write_all(sockets[0], &reply_header, sizeof(reply_header)) == 0 &&
       write_all(sockets[0], &reply, reply_size) == 0;
  close(sockets[0]);

  int status = 0;
  ok = ok && waitpid(child, &status, 0) == child &&
       WIFEXITED(status) && WEXITSTATUS(status) == 0;
  return ok;
}

int main(void) {
  if (!rejects_hello_reply(sizeof(lcom_hello_reply_t) - 1, LCOM_PROTOCOL_VERSION)) {
    fprintf(stderr, "client accepted a truncated hello reply\n");
    return 1;
  }
  if (!rejects_hello_reply(sizeof(lcom_hello_reply_t), LCOM_PROTOCOL_VERSION + 1)) {
    fprintf(stderr, "client accepted a mismatched protocol version\n");
    return 1;
  }
  puts("client protocol tests passed");
  return 0;
}
