#include "../runtime/core/Machine.hpp"
#include "../runtime/core/MousePacketScheduler.hpp"
#include "../runtime/core/Script.hpp"
#include "../runtime/core/ProtocolIO.hpp"
#include "../runtime/backends/AudioBackend.hpp"

#include <lcom/i8042.h>
#include <lcom/i8254.h>
#include <lcom/ac97.h>
#include <lcom/rtc.h>
#include <lcom/uart16550.h>
#include <lcom/vbe.h>

#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

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

static void test_i8042_reasserts_buffered_irq_on_subscribe() {
  lcom::Machine machine;
  machine.injectKey("A", true);
  CHECK(machine.pendingIrqs() == 0);

  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(KBC_IRQ, sub));
  CHECK(machine.pendingIrqs() == BIT(KBC_IRQ));

  uint8_t data = 0;
  CHECK(machine.readPort8(KBC_OUT_BUF, data));
  CHECK(data == 0x1E);
}

static void test_i8042_keyboard_common_scancodes() {
  lcom::Machine machine;
  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(KBC_IRQ, sub));

  auto expect_byte = [&](uint8_t expected) {
    uint8_t data = 0;
    CHECK(machine.readPort8(KBC_OUT_BUF, data));
    CHECK(data == expected);
  };

  machine.injectKey("ENTER", true);
  expect_byte(0x1C);
  machine.injectKey("ENTER", false);
  expect_byte(0x9C);
  machine.injectKey("BACKSPACE", true);
  expect_byte(0x0E);
  machine.injectKey("MINUS", true);
  expect_byte(0x0C);
  machine.injectKey("F12", true);
  expect_byte(0x58);

  machine.injectKey("LEFT", true);
  expect_byte(0xE0);
  expect_byte(0x4B);
  machine.injectKey("DELETE", false);
  expect_byte(0xE0);
  expect_byte(0xD3);
  machine.injectKey("RCTRL", true);
  expect_byte(0xE0);
  expect_byte(0x1D);
}

static void test_mouse_scheduler_coalesces_motion() {
  lcom::MousePacketScheduler scheduler;
  for (int i = 0; i < 10000; i++) scheduler.addMotion(1, 1);

  auto packet = scheduler.nextPacket(false);
  CHECK(packet.has_value());
  CHECK(packet->dx == 96);
  CHECK(packet->dy == -96);
  CHECK(!scheduler.nextPacket(false).has_value());
}

static void test_mouse_scheduler_clamps_and_flips_y() {
  lcom::MousePacketScheduler scheduler;
  scheduler.addMotion(-400, 300);
  auto packet = scheduler.nextPacket(false);
  CHECK(packet.has_value());
  CHECK(packet->dx == -96);
  CHECK(packet->dy == -96);
}

static void test_mouse_scheduler_button_change_emits_zero_delta() {
  lcom::MousePacketScheduler scheduler;
  CHECK(!scheduler.nextPacket(false).has_value());
  scheduler.setButtons(0x01);
  auto packet = scheduler.nextPacket(true);
  CHECK(packet.has_value());
  CHECK(packet->dx == 0);
  CHECK(packet->dy == 0);
  CHECK(packet->buttons == 0x01);
  CHECK(!scheduler.nextPacket(false).has_value());
}

static void test_mouse_scheduler_bounds_backlog() {
  lcom::MousePacketScheduler scheduler;
  scheduler.addMotion(50000, -50000);
  CHECK(scheduler.pendingDx() == 96);
  CHECK(scheduler.pendingScreenDy() == -96);
  auto packet = scheduler.nextPacket(false);
  CHECK(packet.has_value());
  CHECK(packet->dx == 96);
  CHECK(packet->dy == 96);
  CHECK(scheduler.pendingDx() == 0);
  CHECK(scheduler.pendingScreenDy() == 0);
  CHECK(!scheduler.nextPacket(false).has_value());
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
  CHECK(!machine.vbe().ownsRange(std::numeric_limits<uint64_t>::max(), 2));
  CHECK(!machine.vbe().ownsRange(info.framebuffer_phys, std::numeric_limits<uint64_t>::max()));
  machine.vbe().framebuffer()[0] = 0xAA;
  CHECK(machine.vbe().framebuffer()[0] == 0xAA);
}

static uint32_t read_u32_le(const std::vector<uint8_t> &bytes, size_t offset) {
  return static_cast<uint32_t>(bytes[offset]) |
         (static_cast<uint32_t>(bytes[offset + 1]) << 8) |
         (static_cast<uint32_t>(bytes[offset + 2]) << 16) |
         (static_cast<uint32_t>(bytes[offset + 3]) << 24);
}

static void test_wav_backend_streams_multiple_blocks() {
  const char *path = "/tmp/machine-lab-streaming-audio.wav";
  std::remove(path);
  const int16_t first[] = {1, -1, 2, -2};
  const int16_t second[] = {3, -3};
  std::string error;
  {
    lcom::WavAudioBackend wav(path);
    CHECK(wav.playPcm16(first, 2, 48000, 2, error));
    wav.stop();
    CHECK(wav.playPcm16(second, 1, 48000, 2, error));
  }

  std::ifstream input(path, std::ios::binary);
  std::vector<uint8_t> bytes((std::istreambuf_iterator<char>(input)),
                             std::istreambuf_iterator<char>());
  CHECK(bytes.size() == 44 + sizeof(first) + sizeof(second));
  if (bytes.size() >= 44) {
    CHECK(std::string(bytes.begin(), bytes.begin() + 4) == "RIFF");
    CHECK(std::string(bytes.begin() + 8, bytes.begin() + 12) == "WAVE");
    CHECK(read_u32_le(bytes, 4) == bytes.size() - 8);
    CHECK(read_u32_le(bytes, 40) == bytes.size() - 44);
  }

  lcom::WavAudioBackend invalid("/missing-machine-lab-dir/audio.wav");
  CHECK(!invalid.playPcm16(first, 2, 48000, 2, error));
}

static void test_protocol_decode_accepts_unaligned_storage() {
  lcom_port_write8_t expected{};
  expected.port = 0x3F8;
  expected.value = 0x5A;
  std::vector<uint8_t> storage(sizeof(expected) + 1);
  std::memcpy(storage.data() + 1, &expected, sizeof(expected));
  lcom_port_write8_t decoded{};
  CHECK(lcom::protocol::decodePayload(storage.data() + 1, sizeof(expected), decoded));
  CHECK(decoded.port == expected.port);
  CHECK(decoded.value == expected.value);
  CHECK(!lcom::protocol::decodePayload(storage.data() + 1, sizeof(expected) - 1, decoded));
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

static void test_uart_virtual_wire_preserves_bursts() {
  lcom::Machine machine;
  CHECK(machine.writePort8(COM2_BASE + SER_FCR, FCR_ENABLE_FIFO | FCR_CLEAR_RX | FCR_CLEAR_TX));
  CHECK(machine.writePort8(COM2_BASE + SER_IER, IER_RDA));

  for (uint8_t i = 0; i < 64; i++) {
    CHECK(machine.writePort8(COM1_BASE + SER_THR, static_cast<uint8_t>('A' + (i % 26))));
  }

  lcom::IrqSubscription sub;
  CHECK(machine.subscribeIrq(COM2_IRQ, sub));
  CHECK((machine.pendingIrqs() & BIT(COM2_IRQ)) != 0);

  for (uint8_t i = 0; i < 64; i++) {
    uint8_t lsr = 0;
    CHECK(machine.readPort8(COM2_BASE + SER_LSR, lsr));
    CHECK((lsr & LSR_RX_RDY) != 0);
    uint8_t data = 0;
    CHECK(machine.readPort8(COM2_BASE + SER_RBR, data));
    CHECK(data == static_cast<uint8_t>('A' + (i % 26)));
  }
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

static void test_script_time_and_demo_events() {
  const char *path = "/tmp/machine-lab-script-unit.mlabscript";
  {
    std::ofstream out(path);
    out << "at 1s caption top 2s HELLO LEVEL 1\n";
    out << "at 2s out\n";
    out << "at 2.5s in\n";
    out << "at 2.75s caption 1s bottom SECOND LINE\n";
    out << "at 3s move 10 -5 during 5\n";
  }

  lcom::Script script;
  std::string error;
  CHECK(script.load(path, error));

  auto due60 = script.takeDue(60);
  CHECK(due60.size() == 1);
  CHECK(due60[0].kind == lcom::ScriptEvent::Kind::Caption);
  CHECK(due60[0].duration == 120);
  CHECK(due60[0].caption_position == "top");

  auto due120 = script.takeDue(120);
  CHECK(due120.size() == 1);
  CHECK(due120[0].kind == lcom::ScriptEvent::Kind::Capture);
  CHECK(!due120[0].capture_enabled);

  auto due150 = script.takeDue(150);
  CHECK(due150.size() == 1);
  CHECK(due150[0].kind == lcom::ScriptEvent::Kind::Capture);
  CHECK(due150[0].capture_enabled);

  auto due165 = script.takeDue(165);
  CHECK(due165.size() == 1);
  CHECK(due165[0].kind == lcom::ScriptEvent::Kind::Caption);
  CHECK(due165[0].caption_position == "bottom");

  int total_dx = 0;
  int total_dy = 0;
  int move_events = 0;
  for (uint64_t tick = 180; tick < 185; tick++) {
    for (const auto &ev : script.takeDue(tick)) {
      if (ev.kind == lcom::ScriptEvent::Kind::Mouse) {
        total_dx += ev.dx;
        total_dy += ev.dy;
        move_events++;
      }
    }
  }
  CHECK(move_events == 5);
  CHECK(total_dx == 10);
  CHECK(total_dy == -5);
}

int main() {
  test_irq_controller_shape();
  test_pit_programming_and_readback();
  test_i8042_keyboard_irq_and_ports();
  test_i8042_reasserts_buffered_irq_on_subscribe();
  test_i8042_keyboard_common_scancodes();
  test_mouse_scheduler_coalesces_motion();
  test_mouse_scheduler_clamps_and_flips_y();
  test_mouse_scheduler_button_change_emits_zero_delta();
  test_mouse_scheduler_bounds_backlog();
  test_i8042_command_byte_and_mouse_packet();
  test_rtc_cmos_bcd_registers();
  test_vbe_mode_and_framebuffer();
  test_wav_backend_streams_multiple_blocks();
  test_protocol_decode_accepts_unaligned_storage();
  test_uart_loopback_registers();
  test_uart_pair_virtual_cable();
  test_uart_virtual_wire_preserves_bursts();
  test_ac97_registers_and_buffer();
  test_script_time_and_demo_events();

  if (g_failures != 0) {
    std::cerr << g_failures << " unit checks failed\n";
    return 1;
  }
  std::cout << "unit tests passed\n";
  return 0;
}
