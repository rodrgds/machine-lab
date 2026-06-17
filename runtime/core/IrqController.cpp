#include "IrqController.hpp"

namespace lcom {

bool IrqController::subscribe(uint8_t irq, IrqSubscription &out) {
  if (irq >= 32) return false;
  subscribed_[irq] = true;
  out.irq = irq;
  out.bit_no = irq;
  out.mask = (1u << irq);
  return true;
}

bool IrqController::unsubscribe(uint8_t irq) {
  if (irq >= 32) return false;
  subscribed_[irq] = false;
  pending_mask_ &= ~(1u << irq);
  return true;
}

void IrqController::raise(uint8_t irq) {
  if (irq >= 32) return;
  if (!subscribed_[irq]) return;
  pending_mask_ |= (1u << irq);
}

uint32_t IrqController::consumePending() {
  uint32_t mask = pending_mask_;
  pending_mask_ = 0;
  return mask;
}

bool IrqController::isSubscribed(uint8_t irq) const {
  return irq < 32 && subscribed_[irq];
}

} // namespace lcom
