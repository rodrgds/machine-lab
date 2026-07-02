#ifndef MACHINE_LAB_IRQ_CONTROLLER_HPP
#define MACHINE_LAB_IRQ_CONTROLLER_HPP

#include <array>
#include <cstdint>

namespace lcom {

struct IrqSubscription {
  uint8_t irq = 0;
  uint8_t bit_no = 0;
  uint32_t mask = 0;
};

class IrqController {
public:
  bool subscribe(uint8_t irq, IrqSubscription &out);
  bool unsubscribe(uint8_t irq);
  void raise(uint8_t irq);
  uint32_t consumePending();
  uint32_t pendingMask() const { return pending_mask_; }
  bool isSubscribed(uint8_t irq) const;

private:
  std::array<bool, 32> subscribed_{};
  uint32_t pending_mask_ = 0;
};

} // namespace lcom

#endif
