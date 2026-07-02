#ifndef MACHINE_LAB_BUS_HPP
#define MACHINE_LAB_BUS_HPP

#include "PortDevice.hpp"

#include <cstdint>
#include <vector>

namespace lcom {

class Bus {
public:
  void attach(PortDevice *device);
  bool read8(uint16_t port, uint8_t &value);
  bool write8(uint16_t port, uint8_t value);

private:
  std::vector<PortDevice *> devices_;
};

} // namespace lcom

#endif
