#ifndef MACHINE_LAB_PORT_DEVICE_HPP
#define MACHINE_LAB_PORT_DEVICE_HPP

#include <cstdint>

namespace lcom {

class PortDevice {
public:
  virtual ~PortDevice() = default;
  virtual bool handles(uint16_t port) const = 0;
  virtual bool read8(uint16_t port, uint8_t &value) = 0;
  virtual bool write8(uint16_t port, uint8_t value) = 0;
};

} // namespace lcom

#endif
