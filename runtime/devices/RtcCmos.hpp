#ifndef MACHINE_LAB_RTC_CMOS_HPP
#define MACHINE_LAB_RTC_CMOS_HPP

#include "../core/PortDevice.hpp"

#include <cstdint>
#include <ctime>
#include <string>

namespace lcom {

class RtcCmos : public PortDevice {
public:
  RtcCmos();

  bool handles(uint16_t port) const override;
  bool read8(uint16_t port, uint8_t &value) override;
  bool write8(uint16_t port, uint8_t value) override;

  void setHostTime();
  bool setIsoTime(const std::string &iso);
  void setBinaryMode(bool binary);
  void setUpdateInProgress(bool uip) { update_in_progress_ = uip; }

private:
  uint8_t readRegister(uint8_t reg) const;
  static uint8_t toBcd(uint8_t value);

  uint8_t selected_ = 0;
  std::tm tm_{};
  bool binary_mode_ = false;
  bool update_in_progress_ = false;
};

} // namespace lcom

#endif
