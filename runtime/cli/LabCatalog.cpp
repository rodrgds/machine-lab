#include "LabCatalog.hpp"

namespace lcom::cli {

const std::vector<StudentLabSpec> &studentLabSpecs() {
  static const std::vector<StudentLabSpec> specs = {
      {"lab1", "rtc", "Bitwise helpers and RTC/CMOS", {"rtc", "bitwise"},
       {"bitwise.c", "rtc_lab.c"},
       {"uint8_t bit_clear(uint8_t value, uint8_t bit)",
        "uint8_t bit_set(uint8_t value, uint8_t bit)",
        "int bit_is_set(uint8_t value, uint8_t bit)",
        "uint8_t bit_lsb(uint16_t value)",
        "uint8_t bit_msb(uint16_t value)",
        "uint8_t bit_mask(unsigned first_bit, ...)",
        "int rtc_read_date(lcom_rtc_date_t *date)",
        "int rtc_read_time(lcom_rtc_time_t *time)"}},
      {"lab2", "timer", "i8254 PIT and IRQ0 timer events", {"timer", "pit"},
       {"timer_lab.c"},
       {"int timer_set_frequency(uint8_t timer, uint32_t freq)",
        "int timer_get_conf(uint8_t timer, uint8_t *status)",
        "int timer_subscribe(lcom_irq_t *irq)",
        "int timer_unsubscribe(lcom_irq_t *irq)",
        "void timer_ih(void)",
        "uint32_t timer_ticks(void)"}},
      {"lab3", "kbd", "i8042 keyboard and PS/2 scancodes", {"kbd", "keyboard", "kbc"},
       {"keyboard_lab.c"},
       {"int kbc_read_status(uint8_t *status)",
        "int kbc_read_output(uint8_t *byte)",
        "int kbc_write_command(uint8_t command)",
        "int kbd_process_byte(uint8_t byte)",
        "int kbd_get_scancode(uint8_t bytes[2], uint8_t *size, int *make)"}},
      {"lab4", "mouse", "PS/2 mouse packets", {"mouse"},
       {"mouse_lab.c"},
       {"int mouse_enable_data_reporting(void)",
        "int mouse_disable_data_reporting(void)",
        "int mouse_process_byte(uint8_t byte)",
        "int mouse_get_packet(mouse_packet_t *packet)"}},
      {"lab5", "graphics", "VBE framebuffer graphics and XPM sprites", {"graphics", "video", "vbe"},
       {"graphics_lab.c"},
       {"int video_set_mode(uint16_t mode)",
        "int video_map_framebuffer(void)",
        "int video_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color)",
        "int video_draw_xpm(const char *const *xpm, int16_t x, int16_t y)",
        "int video_present(void)"}},
      {"lab6", "audio", "AC97-lite PCM audio", {"audio", "sound"},
       {"audio_lab.c"},
       {"int audio_map_buffer(void)",
        "int audio_fill_square_wave(uint32_t hz, uint32_t ms)",
        "int audio_play(size_t byte_count)",
        "int audio_stop(void)"}},
      {"lab7", "uart", "16550 UART serial ports", {"uart", "serial"},
       {"uart_lab.c"},
       {"int uart_config(uint16_t base, uint32_t baud, uint8_t line_control)",
        "int uart_enable_fifo(uint16_t base)",
        "int uart_enable_rx_interrupt(uint16_t base)",
        "int uart_set_loopback(uint16_t base, int enabled)",
        "int uart_send_byte(uint16_t base, uint8_t byte)",
        "int uart_read_byte(uint16_t base, uint8_t *byte)",
        "int uart_subscribe(uint8_t irq, lcom_irq_t *out)",
        "int uart_unsubscribe(lcom_irq_t *irq)"}},
  };
  return specs;
}

const StudentLabSpec *studentLabSpec(const std::string &name) {
  for (const StudentLabSpec &spec : studentLabSpecs()) {
    if (name == spec.id || name == spec.dir) return &spec;
    for (const std::string &alias : spec.aliases) {
      if (name == alias) return &spec;
    }
  }
  return nullptr;
}

} // namespace lcom::cli
