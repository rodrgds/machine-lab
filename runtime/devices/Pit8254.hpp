#ifndef MACHINE_LAB_PIT8254_HPP
#define MACHINE_LAB_PIT8254_HPP

#include "../core/IrqController.hpp"
#include "../core/PortDevice.hpp"

#include <array>
#include <cstdint>

namespace lcom {

class Pit8254 : public PortDevice {
public:
  explicit Pit8254(IrqController &irqs);

  bool handles(uint16_t port) const override;
  bool read8(uint16_t port, uint8_t &value) override;
  bool write8(uint16_t port, uint8_t value) override;

  void advanceTick();
  uint32_t channelFrequency(uint8_t channel) const;
  uint16_t divisor(uint8_t channel) const;
  uint8_t status(uint8_t channel) const;

private:
  struct Channel {
    uint16_t divisor = 19886;
    uint8_t access = 0x30;
    uint8_t mode = 3;
    bool bcd = false;
    bool expect_msb = false;
    uint8_t pending_lsb = 0;
    bool latched_status_valid = false;
    uint8_t latched_status = 0;
  };

  static int channelFromPort(uint16_t port);
  void writeControl(uint8_t value);
  uint8_t composeStatus(uint8_t channel) const;

  IrqController &irqs_;
  std::array<Channel, 3> channels_{};
};

} // namespace lcom

#endif
