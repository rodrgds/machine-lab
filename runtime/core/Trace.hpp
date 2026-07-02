#ifndef MACHINE_LAB_TRACE_HPP
#define MACHINE_LAB_TRACE_HPP

#include <cstdint>
#include <fstream>
#include <string>

namespace lcom {

class Trace {
public:
  Trace() = default;
  explicit Trace(const std::string &path);

  bool open(const std::string &path);
  void log(const std::string &event, const std::string &detail);
  void logPortRead(uint64_t tick, uint16_t port, uint8_t value, bool ok);
  void logPortWrite(uint64_t tick, uint16_t port, uint8_t value, bool ok);
  void logIrq(uint64_t tick, uint8_t irq, uint32_t mask);

private:
  std::ofstream out_;
};

} // namespace lcom

#endif
