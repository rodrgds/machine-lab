#ifndef MACHINE_LAB_MACHINE_HPP
#define MACHINE_LAB_MACHINE_HPP

#include "Bus.hpp"
#include "IrqController.hpp"
#include "Trace.hpp"
#include "../devices/Ac97.hpp"
#include "../devices/I8042.hpp"
#include "../devices/Pit8254.hpp"
#include "../devices/RtcCmos.hpp"
#include "../devices/Uart16550.hpp"
#include "../devices/Vbe.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace lcom {

class InputObserver {
public:
  virtual ~InputObserver() = default;
  virtual void onKeyScancode(uint8_t byte) = 0;
  virtual void onMousePacket(int dx, int dy, uint8_t buttons) = 0;
};

class Machine {
public:
  Machine();

  bool readPort8(uint16_t port, uint8_t &value);
  bool writePort8(uint16_t port, uint8_t value);

  bool subscribeIrq(uint8_t irq, IrqSubscription &out);
  bool unsubscribeIrq(uint8_t irq);
  uint32_t consumePendingIrqs();
  uint32_t pendingIrqs() const;
  void refreshDeviceIrqs();

  void advanceTick();
  uint64_t tick() const { return tick_; }

  void injectKey(const std::string &key, bool pressed);
  void injectKeyBytes(const std::vector<uint8_t> &bytes);
  void injectMouse(int dx, int dy, uint8_t buttons);
  void connectSerialPeer(Machine *peer);

  Pit8254 &pit() { return pit_; }
  I8042 &i8042() { return i8042_; }
  RtcCmos &rtc() { return rtc_; }
  Vbe &vbe() { return vbe_; }
  Ac97 &ac97() { return ac97_; }
  Uart16550 &com1() { return com1_; }
  Uart16550 &com2() { return com2_; }

  void setTrace(Trace *trace) { trace_ = trace; }
  void setInputObserver(InputObserver *observer) { input_observer_ = observer; }

private:
  uint64_t tick_ = 0;
  IrqController irqs_;
  Bus bus_;
  Pit8254 pit_;
  I8042 i8042_;
  RtcCmos rtc_;
  Vbe vbe_;
  Ac97 ac97_;
  Uart16550 com1_;
  Uart16550 com2_;
  Trace *trace_ = nullptr;
  InputObserver *input_observer_ = nullptr;
};

} // namespace lcom

#endif
