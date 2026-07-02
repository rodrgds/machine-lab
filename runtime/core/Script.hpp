#ifndef MACHINE_LAB_SCRIPT_HPP
#define MACHINE_LAB_SCRIPT_HPP

#include "Machine.hpp"

#include <cstdint>
#include <string>
#include <vector>

namespace lcom {

struct ScriptEvent {
  uint64_t at = 0;
  uint64_t duration = 0;
  enum class Kind {
    Key,
    Mouse,
    Text,
    Rtc,
    Tick,
    Caption,
    Capture
  } kind = Kind::Tick;
  std::string key;
  bool pressed = false;
  int dx = 0;
  int dy = 0;
  uint8_t buttons = 0;
  std::string text;
  std::string caption_position = "bottom";
  bool capture_enabled = true;
};

class Script {
public:
  bool load(const std::string &path, std::string &error);
  bool empty() const { return events_.empty(); }
  void injectDue(Machine &machine, uint64_t tick);
  std::vector<ScriptEvent> takeDue(uint64_t tick);
  uint64_t nextTick() const;

private:
  std::vector<ScriptEvent> events_;
  size_t cursor_ = 0;
};

} // namespace lcom

#endif
