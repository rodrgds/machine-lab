#include "Pit8254.hpp"

#include <lcom/i8254.h>

namespace lcom {

Pit8254::Pit8254(IrqController &irqs) : irqs_(irqs) {}

bool Pit8254::handles(uint16_t port) const {
  return port >= TIMER_0 && port <= TIMER_CTRL;
}

int Pit8254::channelFromPort(uint16_t port) {
  if (port >= TIMER_0 && port <= TIMER_2) return static_cast<int>(port - TIMER_0);
  return -1;
}

bool Pit8254::read8(uint16_t port, uint8_t &value) {
  int channel = channelFromPort(port);
  if (channel < 0) return false;

  Channel &ch = channels_[static_cast<size_t>(channel)];
  if (ch.latched_status_valid) {
    value = ch.latched_status;
    ch.latched_status_valid = false;
    return true;
  }

  if (!ch.expect_msb) {
    value = static_cast<uint8_t>(ch.divisor & 0xFF);
    ch.expect_msb = true;
  } else {
    value = static_cast<uint8_t>(ch.divisor >> 8);
    ch.expect_msb = false;
  }
  return true;
}

bool Pit8254::write8(uint16_t port, uint8_t value) {
  if (port == TIMER_CTRL) {
    writeControl(value);
    return true;
  }

  int channel = channelFromPort(port);
  if (channel < 0) return false;
  Channel &ch = channels_[static_cast<size_t>(channel)];

  switch (ch.access) {
  case TIMER_LSB:
    ch.divisor = static_cast<uint16_t>((ch.divisor & 0xFF00u) | value);
    break;
  case TIMER_MSB:
    ch.divisor = static_cast<uint16_t>((ch.divisor & 0x00FFu) | (static_cast<uint16_t>(value) << 8));
    break;
  case TIMER_LSB_MSB:
    if (!ch.expect_msb) {
      ch.pending_lsb = value;
      ch.expect_msb = true;
    } else {
      ch.divisor = static_cast<uint16_t>((static_cast<uint16_t>(value) << 8) | ch.pending_lsb);
      ch.expect_msb = false;
    }
    break;
  default:
    break;
  }

  return true;
}

void Pit8254::writeControl(uint8_t value) {
  if ((value & TIMER_RB_CMD) == TIMER_RB_CMD) {
    bool latch_status = (value & TIMER_RB_STATUS_) == 0;
    for (uint8_t ch = 0; ch < 3; ch++) {
      if ((value & TIMER_RB_SEL(ch)) != 0 && latch_status) {
        channels_[ch].latched_status = composeStatus(ch);
        channels_[ch].latched_status_valid = true;
      }
    }
    return;
  }

  uint8_t selected = static_cast<uint8_t>((value >> 6) & 0x03u);
  if (selected > 2) return;

  Channel &ch = channels_[selected];
  ch.access = value & 0x30u;
  ch.mode = static_cast<uint8_t>((value >> 1) & 0x07u);
  if (ch.mode == 6) ch.mode = 2;
  if (ch.mode == 7) ch.mode = 3;
  ch.bcd = (value & TIMER_BCD) != 0;
  ch.expect_msb = false;
}

uint8_t Pit8254::composeStatus(uint8_t channel) const {
  const Channel &ch = channels_[channel];
  return static_cast<uint8_t>(0x80u | ch.access | ((ch.mode & 0x07u) << 1) | (ch.bcd ? 1u : 0u));
}

void Pit8254::advanceTick() {
  irqs_.raise(TIMER0_IRQ);
}

uint32_t Pit8254::channelFrequency(uint8_t channel) const {
  if (channel >= channels_.size()) return 0;
  uint16_t div = channels_[channel].divisor;
  uint32_t effective_divisor = div == 0 ? 65536u : div;
  return TIMER_FREQ / effective_divisor;
}

uint16_t Pit8254::divisor(uint8_t channel) const {
  if (channel >= channels_.size()) return 0;
  return channels_[channel].divisor;
}

uint8_t Pit8254::status(uint8_t channel) const {
  if (channel >= channels_.size()) return 0;
  return composeStatus(channel);
}

} // namespace lcom
