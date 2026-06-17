#ifndef LCOM_NG_MOUSE_PACKET_SCHEDULER_HPP
#define LCOM_NG_MOUSE_PACKET_SCHEDULER_HPP

#include <algorithm>
#include <cstdint>
#include <optional>

namespace lcom {

struct MousePacket {
  int dx = 0;
  int dy = 0;
  uint8_t buttons = 0;
};

class MousePacketScheduler {
public:
  void reset() {
    pending_dx_ = 0;
    pending_screen_dy_ = 0;
    buttons_ = 0;
    button_changed_ = false;
  }

  void addMotion(int dx, int screen_dy) {
    pending_dx_ = clamp(pending_dx_ + dx, -kBacklogLimit, kBacklogLimit);
    pending_screen_dy_ = clamp(pending_screen_dy_ + screen_dy,
                               -kBacklogLimit, kBacklogLimit);
  }

  void setButtons(uint8_t buttons) {
    buttons &= 0x07u;
    if (buttons_ != buttons) {
      buttons_ = buttons;
      button_changed_ = true;
    }
  }

  std::optional<MousePacket> nextPacket(bool force) {
    if (!force && pending_dx_ == 0 && pending_screen_dy_ == 0 && !button_changed_) {
      return std::nullopt;
    }

    int dx = clamp(pending_dx_, -kPacketLimit, kPacketLimit);
    int screen_dy = clamp(pending_screen_dy_, -kPacketLimit, kPacketLimit);
    if (dx == 0 && screen_dy == 0 && !button_changed_) return std::nullopt;

    pending_dx_ -= dx;
    pending_screen_dy_ -= screen_dy;
    button_changed_ = false;

    return MousePacket{dx, -screen_dy, buttons_};
  }

  int pendingDx() const { return pending_dx_; }
  int pendingScreenDy() const { return pending_screen_dy_; }

private:
  static constexpr int kPacketLimit = 255;
  static constexpr int kBacklogLimit = 1024;

  static int clamp(int value, int lo, int hi) {
    return std::max(lo, std::min(hi, value));
  }

  int pending_dx_ = 0;
  int pending_screen_dy_ = 0;
  uint8_t buttons_ = 0;
  bool button_changed_ = false;
};

} // namespace lcom

#endif
