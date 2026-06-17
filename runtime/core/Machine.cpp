#include "Machine.hpp"

#include <lcom/uart16550.h>

namespace lcom {

Machine::Machine()
    : pit_(irqs_), i8042_(irqs_), rtc_(), vbe_(),
      com1_(COM1_BASE, COM1_IRQ, irqs_), com2_(COM2_BASE, COM2_IRQ, irqs_) {
  com1_.connectPeer(&com2_);
  com2_.connectPeer(&com1_);
  bus_.attach(&pit_);
  bus_.attach(&i8042_);
  bus_.attach(&rtc_);
  bus_.attach(&ac97_);
  bus_.attach(&com1_);
  bus_.attach(&com2_);
}

bool Machine::readPort8(uint16_t port, uint8_t &value) {
  bool ok = bus_.read8(port, value);
  if (trace_ != nullptr) trace_->logPortRead(tick_, port, ok ? value : 0, ok);
  return ok;
}

bool Machine::writePort8(uint16_t port, uint8_t value) {
  bool ok = bus_.write8(port, value);
  if (trace_ != nullptr) trace_->logPortWrite(tick_, port, value, ok);
  return ok;
}

bool Machine::subscribeIrq(uint8_t irq, IrqSubscription &out) {
  bool ok = irqs_.subscribe(irq, out);
  if (ok) refreshDeviceIrqs();
  return ok;
}

bool Machine::unsubscribeIrq(uint8_t irq) {
  return irqs_.unsubscribe(irq);
}

uint32_t Machine::consumePendingIrqs() {
  return irqs_.consumePending();
}

uint32_t Machine::pendingIrqs() const {
  return irqs_.pendingMask();
}

void Machine::refreshDeviceIrqs() {
  i8042_.refreshIrq();
}

void Machine::advanceTick() {
  tick_++;
  pit_.advanceTick();
  if (trace_ != nullptr && pendingIrqs() != 0) {
    for (uint8_t irq = 0; irq < 32; irq++) {
      uint32_t mask = (1u << irq);
      if ((pendingIrqs() & mask) != 0) trace_->logIrq(tick_, irq, mask);
    }
  }
}

void Machine::injectKey(const std::string &key, bool pressed) {
  injectKeyBytes(ps2Set1ForKey(key, pressed));
}

void Machine::injectKeyBytes(const std::vector<uint8_t> &bytes) {
  i8042_.injectKeySequence(bytes);
  if (trace_ != nullptr && pendingIrqs() != 0) {
    trace_->log("input", "keyboard");
  }
}

void Machine::injectMouse(int dx, int dy, uint8_t buttons) {
  i8042_.injectMousePacket(dx, dy, buttons);
  if (trace_ != nullptr && pendingIrqs() != 0) {
    trace_->log("input", "mouse");
  }
}

} // namespace lcom
