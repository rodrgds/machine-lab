#include "Trace.hpp"

#include <iomanip>
#include <sstream>

namespace lcom {

Trace::Trace(const std::string &path) {
  open(path);
}

bool Trace::open(const std::string &path) {
  out_.open(path, std::ios::out | std::ios::trunc);
  return out_.is_open();
}

void Trace::log(const std::string &event, const std::string &detail) {
  if (!out_.is_open()) return;
  out_ << "{\"event\":\"" << event << "\",\"detail\":\"";
  for (char c : detail) {
    if (c == '"' || c == '\\') out_ << '\\';
    if (c == '\n') out_ << "\\n";
    else out_ << c;
  }
  out_ << "\"}\n";
}

void Trace::logPortRead(uint64_t tick, uint16_t port, uint8_t value, bool ok) {
  if (!out_.is_open()) return;
  out_ << "{\"tick\":" << tick << ",\"event\":\"port_read8\",\"port\":\"0x"
       << std::hex << std::setw(4) << std::setfill('0') << port
       << "\",\"value\":\"0x" << std::setw(2) << static_cast<unsigned>(value)
       << "\",\"ok\":" << std::dec << (ok ? "true" : "false") << "}\n";
}

void Trace::logPortWrite(uint64_t tick, uint16_t port, uint8_t value, bool ok) {
  if (!out_.is_open()) return;
  out_ << "{\"tick\":" << tick << ",\"event\":\"port_write8\",\"port\":\"0x"
       << std::hex << std::setw(4) << std::setfill('0') << port
       << "\",\"value\":\"0x" << std::setw(2) << static_cast<unsigned>(value)
       << "\",\"ok\":" << std::dec << (ok ? "true" : "false") << "}\n";
}

void Trace::logIrq(uint64_t tick, uint8_t irq, uint32_t mask) {
  if (!out_.is_open()) return;
  out_ << "{\"tick\":" << tick << ",\"event\":\"irq\",\"irq\":"
       << static_cast<unsigned>(irq) << ",\"mask\":" << mask << "}\n";
}

} // namespace lcom
