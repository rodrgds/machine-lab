#include "../runtime/core/Machine.hpp"

#include <lcom/i8042.h>
#include <lcom/i8254.h>
#include <lcom/ac97.h>
#include <lcom/rtc.h>
#include <lcom/uart16550.h>
#include <lcom/vbe.h>

#include <cstdlib>
#include <iostream>
#include <string>

static int g_failures = 0;

#define CHECK(expr)                                                                  \
  do {                                                                               \
    if (!(expr)) {                                                                   \
      std::cerr << "CHECK failed at " << __FILE__ << ":" << __LINE__ << ": "       \
                << #expr << "\n";                                                   \
      g_failures++;                                                                  \
    }                                                                                \
  } while (0)

static void test_irq_controller_shape() {
  lcom::Machine machine;
  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(TIMER0_IRQ, sub));
  CHECK(sub.irq == TIMER0_IRQ);
  CHECK(sub.bit_no == TIMER0_IRQ);
  CHECK(sub.mask == BIT(TIMER0_IRQ));
  machine.advanceTick();
  CHECK(machine.pendingIrqs() == BIT(TIMER0_IRQ));
  CHECK(machine.consumePendingIrqs() == BIT(TIMER0_IRQ));
  CHECK(machine.pendingIrqs() == 0);
}

static void test_pit_programming_and_readback() {
  lcom::Machine machine;
  CHECK(machine.writePort8(TIMER_CTRL, TIMER_SEL0 | TIMER_LSB_MSB | TIMER_SQR_WAVE | TIMER_BIN));
  CHECK(machine.writePort8(TIMER_0, 0x34));
  CHECK(machine.writePort8(TIMER_0, 0x12));
  CHECK(machine.pit().divisor(0) == 0x1234);
  CHECK(machine.pit().channelFrequency(0) == TIMER_FREQ / 0x1234);

  CHECK(machine.writePort8(TIMER_0, 0x00));
  CHECK(machine.writePort8(TIMER_0, 0x00));
  CHECK(machine.pit().divisor(0) == 0);
  CHECK(machine.pit().channelFrequency(0) == TIMER_FREQ / 65536u);

  CHECK(machine.writePort8(TIMER_CTRL, TIMER_RB_CMD | TIMER_RB_COUNT_ | TIMER_RB_SEL(0)));
  uint8_t status = 0;
  CHECK(machine.readPort8(TIMER_0, status));
  CHECK((status & TIMER_LSB_MSB) == TIMER_LSB_MSB);
  CHECK(((status >> 1) & 0x07) == 3);
}

static void test_i8042_keyboard_irq_and_ports() {
  lcom::Machine machine;
  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(KBC_IRQ, sub));
  machine.injectKey("A", true);
  CHECK(machine.pendingIrqs() == BIT(KBC_IRQ));

  uint8_t status = 0;
  CHECK(machine.readPort8(KBC_ST_REG, status));
  CHECK((status & KBC_ST_OBF) != 0);
  CHECK((status & KBC_ST_AUX) == 0);

  uint8_t data = 0;
  CHECK(machine.readPort8(KBC_OUT_BUF, data));
  CHECK(data == 0x1E);
}

static void test_i8042_command_byte_and_mouse_packet() {
  lcom::Machine machine;
  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(MOUSE_IRQ, sub));

  CHECK(machine.writePort8(KBC_CMD_REG, KBC_CMD_READ_CB));
  uint8_t cb = 0;
  CHECK(machine.readPort8(KBC_OUT_BUF, cb));
  CHECK((cb & KBC_CB_INT) != 0);

  CHECK(machine.writePort8(KBC_CMD_REG, KBC_CMD_WRITE_MOUSE));
  CHECK(machine.writePort8(KBC_IN_BUF, MOUSE_CMD_ENABLE_DR));
  uint8_t ack = 0;
  CHECK(machine.readPort8(KBC_OUT_BUF, ack));
  CHECK(ack == MOUSE_ACK);

  machine.injectMouse(5, -2, 1);
  CHECK(machine.pendingIrqs() == BIT(MOUSE_IRQ));
  uint8_t b0 = 0, b1 = 0, b2 = 0;
  CHECK(machine.readPort8(KBC_OUT_BUF, b0));
  CHECK(machine.readPort8(KBC_OUT_BUF, b1));
  CHECK(machine.readPort8(KBC_OUT_BUF, b2));
  CHECK((b0 & MOUSE_SYNC_BIT) != 0);
  CHECK((b0 & BIT(0)) != 0);
  CHECK((b0 & BIT(5)) != 0);
  CHECK(b1 == 5);
  CHECK(b2 == static_cast<uint8_t>(-2));
}

static void test_rtc_cmos_bcd_registers() {
  lcom::Machine machine;
  CHECK(machine.rtc().setIsoTime("2026-06-16T12:34:56"));
  uint8_t value = 0;
  CHECK(machine.writePort8(RTC_ADDR_REG, RTC_REG_DAY));
  CHECK(machine.readPort8(RTC_DATA_REG, value));
  CHECK(value == 0x16);
  CHECK(machine.writePort8(RTC_ADDR_REG, RTC_REG_MONTH));
  CHECK(machine.readPort8(RTC_DATA_REG, value));
  CHECK(value == 0x06);
  CHECK(machine.writePort8(RTC_ADDR_REG, RTC_REG_YEAR));
  CHECK(machine.readPort8(RTC_DATA_REG, value));
  CHECK(value == 0x26);
  CHECK(machine.writePort8(RTC_ADDR_REG, RTC_REG_B));
  CHECK(machine.readPort8(RTC_DATA_REG, value));
  CHECK((value & RTC_24H) != 0);
  CHECK((value & RTC_DM) == 0);
}

static void test_vbe_mode_and_framebuffer() {
  lcom::Machine machine;
  lcom::VbeModeInfo info;
  CHECK(machine.vbe().modeInfo(LCOM_VBE_MODE_800_600_24, info));
  CHECK(info.width == 800);
  CHECK(info.height == 600);
  CHECK(info.bytes_per_pixel == 3);
  CHECK(machine.vbe().setMode(LCOM_VBE_MODE_800_600_24));
  CHECK(machine.vbe().ownsRange(info.framebuffer_phys, info.framebuffer_size));
  machine.vbe().framebuffer()[0] = 0xAA;
  CHECK(machine.vbe().framebuffer()[0] == 0xAA);
}

static void test_uart_loopback_registers() {
  lcom::Machine machine;
  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(COM1_IRQ, sub));
  CHECK(machine.writePort8(COM1_BASE + SER_LCR, LCR_DLAB));
  CHECK(machine.writePort8(COM1_BASE + SER_DLL, 0x0C));
  CHECK(machine.writePort8(COM1_BASE + SER_DLM, 0x00));
  CHECK(machine.com1().divisor() == 12);
  CHECK(machine.writePort8(COM1_BASE + SER_LCR, 0x03));
  CHECK(machine.writePort8(COM1_BASE + SER_SR, 0x5A));
  uint8_t scratch = 0;
  CHECK(machine.readPort8(COM1_BASE + SER_SR, scratch));
  CHECK(scratch == 0x5A);
  CHECK(machine.writePort8(COM1_BASE + SER_MCR, MCR_LOOP));
  CHECK(machine.writePort8(COM1_BASE + SER_IER, IER_RDA));
  CHECK(machine.writePort8(COM1_BASE + SER_THR, 'X'));
  CHECK(machine.pendingIrqs() == BIT(COM1_IRQ));
  uint8_t lsr = 0;
  CHECK(machine.readPort8(COM1_BASE + SER_LSR, lsr));
  CHECK((lsr & LSR_RX_RDY) != 0);
  uint8_t data = 0;
  CHECK(machine.readPort8(COM1_BASE + SER_RBR, data));
  CHECK(data == 'X');
}

static void test_uart_pair_virtual_cable() {
  lcom::Machine machine;
  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(COM2_IRQ, sub));
  CHECK(machine.writePort8(COM2_BASE + SER_FCR, FCR_ENABLE_FIFO | FCR_CLEAR_RX | FCR_CLEAR_TX));
  CHECK(machine.writePort8(COM2_BASE + SER_IER, IER_RDA));
  CHECK(machine.writePort8(COM1_BASE + SER_THR, 'P'));
  CHECK(machine.pendingIrqs() == BIT(COM2_IRQ));

  uint8_t com1_lsr = 0;
  CHECK(machine.readPort8(COM1_BASE + SER_LSR, com1_lsr));
  CHECK((com1_lsr & LSR_RX_RDY) == 0);

  uint8_t com2_lsr = 0;
  CHECK(machine.readPort8(COM2_BASE + SER_LSR, com2_lsr));
  CHECK((com2_lsr & LSR_RX_RDY) != 0);
  uint8_t data = 0;
  CHECK(machine.readPort8(COM2_BASE + SER_RBR, data));
  CHECK(data == 'P');
}

static void test_ac97_registers_and_buffer() {
  lcom::Machine machine;
  CHECK(machine.ac97().bufferPhys() == 0xD0000000ull);
  CHECK(machine.ac97().bufferSize() >= 1024u * 1024u);
  CHECK(machine.ac97().ownsRange(machine.ac97().bufferPhys(), 4096));

  CHECK(machine.writePort8(AC97_NAM_BASE + AC97_PCM_FRONT_DAC_RATE, 0x80));
  CHECK(machine.writePort8(AC97_NAM_BASE + AC97_PCM_FRONT_DAC_RATE + 1, 0xBB));
  CHECK(machine.ac97().sampleRate() == 48000);

  CHECK(machine.writePort8(AC97_BM_BASE + AC97_PO_CR, AC97_PO_CR_RUN));
  CHECK(machine.ac97().playing());
  uint8_t status = 0;
  CHECK(machine.readPort8(AC97_BM_BASE + AC97_PO_SR, status));
  CHECK((status & AC97_PO_SR_BCIS) != 0);
  CHECK(machine.writePort8(AC97_BM_BASE + AC97_PO_CR, 0));
  CHECK(!machine.ac97().playing());
}

int main() {
  test_irq_controller_shape();
  test_pit_programming_and_readback();
  test_i8042_keyboard_irq_and_ports();
  test_i8042_command_byte_and_mouse_packet();
  test_rtc_cmos_bcd_registers();
  test_vbe_mode_and_framebuffer();
  test_uart_loopback_registers();
  test_uart_pair_virtual_cable();
  test_ac97_registers_and_buffer();

  if (g_failures != 0) {
    std::cerr << g_failures << " unit checks failed\n";
    return 1;
  }
  std::cout << "unit tests passed\n";
  return 0;
}
