#include "RtcCmos.hpp"

#include <lcom/rtc.h>

#include <cstdio>
#include <ctime>

namespace lcom {

RtcCmos::RtcCmos() {
  setHostTime();
}

bool RtcCmos::handles(uint16_t port) const {
  return port == RTC_ADDR_REG || port == RTC_DATA_REG;
}

bool RtcCmos::read8(uint16_t port, uint8_t &value) {
  if (port == RTC_ADDR_REG) {
    value = selected_;
    return true;
  }
  if (port == RTC_DATA_REG) {
    value = readRegister(selected_);
    return true;
  }
  return false;
}

bool RtcCmos::write8(uint16_t port, uint8_t value) {
  if (port == RTC_ADDR_REG) {
    selected_ = static_cast<uint8_t>(value & 0x7Fu);
    return true;
  }
  if (port == RTC_DATA_REG) {
    return true;
  }
  return false;
}

void RtcCmos::setHostTime() {
  std::time_t now = std::time(nullptr);
  std::tm local{};
#if defined(_WIN32)
  localtime_s(&local, &now);
#else
  localtime_r(&now, &local);
#endif
  tm_ = local;
}

bool RtcCmos::setIsoTime(const std::string &iso) {
  int year = 0, month = 0, day = 0, hour = 0, minute = 0, second = 0;
  if (std::sscanf(iso.c_str(), "%d-%d-%dT%d:%d:%d", &year, &month, &day,
                  &hour, &minute, &second) != 6) {
    if (std::sscanf(iso.c_str(), "%d-%d-%d %d:%d:%d", &year, &month, &day,
                    &hour, &minute, &second) != 6) {
      return false;
    }
  }
  tm_ = {};
  tm_.tm_year = year - 1900;
  tm_.tm_mon = month - 1;
  tm_.tm_mday = day;
  tm_.tm_hour = hour;
  tm_.tm_min = minute;
  tm_.tm_sec = second;
  return true;
}

void RtcCmos::setBinaryMode(bool binary) {
  binary_mode_ = binary;
}

uint8_t RtcCmos::toBcd(uint8_t value) {
  return static_cast<uint8_t>(((value / 10u) << 4u) | (value % 10u));
}

uint8_t RtcCmos::readRegister(uint8_t reg) const {
  uint8_t value = 0;
  switch (reg) {
  case RTC_REG_SECONDS:
    value = static_cast<uint8_t>(tm_.tm_sec);
    break;
  case RTC_REG_MINUTES:
    value = static_cast<uint8_t>(tm_.tm_min);
    break;
  case RTC_REG_HOURS:
    value = static_cast<uint8_t>(tm_.tm_hour);
    break;
  case RTC_REG_DAY:
    value = static_cast<uint8_t>(tm_.tm_mday);
    break;
  case RTC_REG_MONTH:
    value = static_cast<uint8_t>(tm_.tm_mon + 1);
    break;
  case RTC_REG_YEAR:
    value = static_cast<uint8_t>(tm_.tm_year % 100);
    break;
  case RTC_REG_A:
    return update_in_progress_ ? RTC_UIP : 0x26u;
  case RTC_REG_B:
    return static_cast<uint8_t>(RTC_24H | (binary_mode_ ? RTC_DM : 0));
  case RTC_REG_C:
  case RTC_REG_D:
    return 0;
  default:
    return 0;
  }

  return binary_mode_ ? value : toBcd(value);
}

} // namespace lcom
