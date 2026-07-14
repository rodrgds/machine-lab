#ifndef MACHINE_LAB_PROTOCOL_IO_HPP
#define MACHINE_LAB_PROTOCOL_IO_HPP

#include "lcom_protocol.h"

#include <cerrno>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <type_traits>
#include <unistd.h>

namespace lcom::protocol {

inline bool writeAll(int fd, const void *buffer, size_t length) {
  const auto *cursor = static_cast<const uint8_t *>(buffer);
  while (length > 0) {
    const ssize_t written = ::write(fd, cursor, length);
    if (written < 0) {
      if (errno == EINTR) continue;
      return false;
    }
    if (written == 0) return false;
    cursor += static_cast<size_t>(written);
    length -= static_cast<size_t>(written);
  }
  return true;
}

inline bool readAll(int fd, void *buffer, size_t length) {
  auto *cursor = static_cast<uint8_t *>(buffer);
  while (length > 0) {
    const ssize_t received = ::read(fd, cursor, length);
    if (received < 0) {
      if (errno == EINTR) continue;
      return false;
    }
    if (received == 0) return false;
    cursor += static_cast<size_t>(received);
    length -= static_cast<size_t>(received);
  }
  return true;
}

inline bool sendMessage(int fd, uint16_t type, uint32_t request_id,
                        const void *payload, uint32_t size) {
  lcom_msg_header_t header{};
  header.type = type;
  header.size = size;
  header.request_id = request_id;
  return writeAll(fd, &header, sizeof(header)) &&
         (size == 0 || writeAll(fd, payload, size));
}

template <typename T>
bool decodePayload(const uint8_t *payload, uint32_t size, T &decoded) {
  static_assert(std::is_trivially_copyable<T>::value,
                "protocol payloads must be trivially copyable");
  if (size != sizeof(T)) return false;
  std::memcpy(&decoded, payload, sizeof(T));
  return true;
}

} // namespace lcom::protocol

#endif
