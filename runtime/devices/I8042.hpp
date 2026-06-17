#ifndef LCOM_NG_I8042_HPP
#define LCOM_NG_I8042_HPP

#include "../core/IrqController.hpp"
#include "../core/PortDevice.hpp"

#include <cstdint>
#include <deque>
#include <string>
#include <vector>

namespace lcom {

class I8042 : public PortDevice {
public:
  explicit I8042(IrqController &irqs);

  bool handles(uint16_t port) const override;
  bool read8(uint16_t port, uint8_t &value) override;
  bool write8(uint16_t port, uint8_t value) override;

  void injectKeyScancode(uint8_t byte);
  void injectKeySequence(const std::vector<uint8_t> &bytes);
  void injectMousePacket(int dx, int dy, uint8_t buttons);
  void refreshIrq();

  uint8_t status() const;
  uint8_t commandByte() const { return command_byte_; }
  bool mouseReporting() const { return mouse_reporting_; }

private:
  struct OutputByte {
    uint8_t value = 0;
    bool aux = false;
  };

  enum class PendingWrite {
    None,
    CommandByte,
    MouseCommand
  };

  void queueOutput(uint8_t value, bool aux);
  void raiseForFront();
  void handleMouseCommand(uint8_t command);

  IrqController &irqs_;
  std::deque<OutputByte> output_;
  uint8_t command_byte_ = 0x03;
  PendingWrite pending_write_ = PendingWrite::None;
  bool keyboard_enabled_ = true;
  bool mouse_reporting_ = false;
};

std::vector<uint8_t> ps2Set1ForKey(const std::string &key, bool pressed);

} // namespace lcom

#endif
