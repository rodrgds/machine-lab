#include "Bus.hpp"

namespace lcom {

void Bus::attach(PortDevice *device) {
  devices_.push_back(device);
}

bool Bus::read8(uint16_t port, uint8_t &value) {
  for (PortDevice *device : devices_) {
    if (device->handles(port)) return device->read8(port, value);
  }
  return false;
}

bool Bus::write8(uint16_t port, uint8_t value) {
  for (PortDevice *device : devices_) {
    if (device->handles(port)) return device->write8(port, value);
  }
  return false;
}

} // namespace lcom
