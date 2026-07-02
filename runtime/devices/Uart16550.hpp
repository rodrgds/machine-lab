#ifndef MACHINE_LAB_UART16550_HPP
#define MACHINE_LAB_UART16550_HPP

#include "../core/IrqController.hpp"
#include "../core/PortDevice.hpp"

#include <cstdint>
#include <deque>

namespace lcom {

class Uart16550 : public PortDevice {
public:
  Uart16550(uint16_t base, uint8_t irq, IrqController &irqs);

  bool handles(uint16_t port) const override;
  bool read8(uint16_t port, uint8_t &value) override;
  bool write8(uint16_t port, uint8_t value) override;

  void connectPeer(Uart16550 *peer);
  void injectRx(uint8_t value);
  bool popTx(uint8_t &value);
  uint16_t divisor() const { return divisor_; }
  void refreshIrq();

private:
  uint8_t readReg(uint8_t reg);
  void writeReg(uint8_t reg, uint8_t value);
  void pumpRx();
  void updateIrq();

  uint16_t base_;
  uint8_t irq_;
  IrqController &irqs_;

  uint8_t ier_ = 0;
  uint8_t iir_ = 0x01;
  uint8_t lcr_ = 0x03;
  uint8_t mcr_ = 0;
  uint8_t msr_ = 0;
  uint8_t scratch_ = 0;
  uint8_t lsr_errors_ = 0;
  uint16_t divisor_ = 1;
  bool fifo_enabled_ = false;
  bool thre_interrupt_pending_ = false;
  Uart16550 *peer_ = nullptr;
  std::deque<uint8_t> rx_;
  std::deque<uint8_t> wire_rx_;
  std::deque<uint8_t> tx_;
};

} // namespace lcom

#endif
