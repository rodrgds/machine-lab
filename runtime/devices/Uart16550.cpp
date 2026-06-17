#include "Uart16550.hpp"

#include <lcom/uart16550.h>

namespace lcom {

Uart16550::Uart16550(uint16_t base, uint8_t irq, IrqController &irqs)
    : base_(base), irq_(irq), irqs_(irqs) {}

void Uart16550::connectPeer(Uart16550 *peer) {
  peer_ = peer;
}

bool Uart16550::handles(uint16_t port) const {
  return port >= base_ && port < base_ + 8;
}

bool Uart16550::read8(uint16_t port, uint8_t &value) {
  if (!handles(port)) return false;
  value = readReg(static_cast<uint8_t>(port - base_));
  return true;
}

bool Uart16550::write8(uint16_t port, uint8_t value) {
  if (!handles(port)) return false;
  writeReg(static_cast<uint8_t>(port - base_), value);
  return true;
}

uint8_t Uart16550::readReg(uint8_t reg) {
  bool dlab = (lcr_ & LCR_DLAB) != 0;
  if (dlab && reg == SER_DLL) return static_cast<uint8_t>(divisor_ & 0xFF);
  if (dlab && reg == SER_DLM) return static_cast<uint8_t>(divisor_ >> 8);

  switch (reg) {
  case SER_RBR: {
    if (rx_.empty()) return 0;
    uint8_t value = rx_.front();
    rx_.pop_front();
    updateIrq();
    return value;
  }
  case SER_IER:
    return ier_;
  case SER_IIR: {
    uint8_t value = iir_;
    if ((value & 0x0Fu) == IIR_THRE) {
      thre_interrupt_pending_ = false;
      updateIrq();
    }
    return value;
  }
  case SER_LCR:
    return lcr_;
  case SER_MCR:
    return mcr_;
  case SER_LSR: {
    uint8_t value = static_cast<uint8_t>(LSR_THRE | LSR_TEMT |
                                         (rx_.empty() ? 0 : LSR_RX_RDY) |
                                         lsr_errors_);
    lsr_errors_ = 0;
    updateIrq();
    return value;
  }
  case SER_MSR:
    return msr_;
  case SER_SR:
    return scratch_;
  default:
    return 0;
  }
}

void Uart16550::writeReg(uint8_t reg, uint8_t value) {
  bool dlab = (lcr_ & LCR_DLAB) != 0;
  if (dlab && reg == SER_DLL) {
    divisor_ = static_cast<uint16_t>((divisor_ & 0xFF00u) | value);
    return;
  }
  if (dlab && reg == SER_DLM) {
    divisor_ = static_cast<uint16_t>((divisor_ & 0x00FFu) | (static_cast<uint16_t>(value) << 8));
    return;
  }

  switch (reg) {
  case SER_THR:
    tx_.push_back(value);
    if ((mcr_ & MCR_LOOP) != 0) {
      injectRx(value);
    } else if (peer_ != nullptr) {
      peer_->injectRx(value);
    }
    thre_interrupt_pending_ = true;
    updateIrq();
    break;
  case SER_IER:
    ier_ = static_cast<uint8_t>(value & 0x0Fu);
    if ((ier_ & IER_THRE) != 0) thre_interrupt_pending_ = true;
    updateIrq();
    break;
  case SER_FCR:
    fifo_enabled_ = (value & FCR_ENABLE_FIFO) != 0;
    if (value & BIT(1)) rx_.clear();
    if (value & BIT(2)) tx_.clear();
    updateIrq();
    break;
  case SER_LCR:
    lcr_ = value;
    break;
  case SER_MCR:
    mcr_ = value;
    break;
  case SER_SR:
    scratch_ = value;
    break;
  default:
    break;
  }
}

void Uart16550::injectRx(uint8_t value) {
  size_t capacity = fifo_enabled_ ? 16u : 1u;
  if (rx_.size() >= capacity) {
    lsr_errors_ |= LSR_OE;
    updateIrq();
    return;
  }
  rx_.push_back(value);
  updateIrq();
}

bool Uart16550::popTx(uint8_t &value) {
  if (tx_.empty()) return false;
  value = tx_.front();
  tx_.pop_front();
  return true;
}

void Uart16550::updateIrq() {
  if (lsr_errors_ != 0 && (ier_ & IER_RLS)) {
    iir_ = 0x06u;
    irqs_.raise(irq_);
  } else if (!rx_.empty() && (ier_ & IER_RDA)) {
    iir_ = IIR_RDA;
    irqs_.raise(irq_);
  } else if (thre_interrupt_pending_ && (ier_ & IER_THRE)) {
    iir_ = IIR_THRE;
    irqs_.raise(irq_);
  } else {
    iir_ = IIR_NO_INT;
  }
}

} // namespace lcom
