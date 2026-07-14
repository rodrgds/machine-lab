#include "Script.hpp"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <sstream>
#include <cstdlib>

namespace lcom {

static std::string trim(const std::string &s) {
  size_t first = 0;
  while (first < s.size() && std::isspace(static_cast<unsigned char>(s[first]))) first++;
  size_t last = s.size();
  while (last > first && std::isspace(static_cast<unsigned char>(s[last - 1]))) last--;
  return s.substr(first, last - first);
}

static bool parseBoolAction(const std::string &word, bool &pressed) {
  if (word == "down" || word == "press" || word == "make") {
    pressed = true;
    return true;
  }
  if (word == "up" || word == "release" || word == "break") {
    pressed = false;
    return true;
  }
  return false;
}

static bool parseTickToken(const std::string &token, uint64_t &tick) {
  if (token.empty()) return false;
  char *end = nullptr;
  double value = std::strtod(token.c_str(), &end);
  if (end == token.c_str() || value < 0.0) return false;
  std::string suffix(end);
  if (suffix.empty()) {
    tick = static_cast<uint64_t>(value);
    return true;
  }
  if (suffix == "s" || suffix == "sec" || suffix == "secs") {
    tick = static_cast<uint64_t>(std::llround(value * 60.0));
    return true;
  }
  if (suffix == "ms") {
    tick = static_cast<uint64_t>(std::llround(value * 60.0 / 1000.0));
    return true;
  }
  return false;
}

static std::string upper(std::string value) {
  std::transform(value.begin(), value.end(), value.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c));
  });
  return value;
}

static bool isCaptionPosition(const std::string &word) {
  return word == "top" || word == "bottom";
}

static void tapKey(Machine &machine, const std::string &key, bool shifted = false) {
  if (shifted) machine.injectKey("LSHIFT", true);
  machine.injectKey(key, true);
  machine.injectKey(key, false);
  if (shifted) machine.injectKey("LSHIFT", false);
}

void injectText(Machine &machine, const std::string &text) {
  for (char c : text) {
    if (c >= 'a' && c <= 'z') {
      tapKey(machine, std::string(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c)))));
    } else if (c >= 'A' && c <= 'Z') {
      tapKey(machine, std::string(1, c), true);
    } else if (c >= '0' && c <= '9') {
      tapKey(machine, std::string(1, c));
    } else if (c == ' ') {
      tapKey(machine, "SPACE");
    } else if (c == '\n') {
      tapKey(machine, "ENTER");
    } else if (c == '-') {
      tapKey(machine, "MINUS");
    } else if (c == '=') {
      tapKey(machine, "EQUALS");
    } else if (c == ',') {
      tapKey(machine, "COMMA");
    } else if (c == '.') {
      tapKey(machine, "PERIOD");
    } else if (c == '/') {
      tapKey(machine, "SLASH");
    } else if (c == ';') {
      tapKey(machine, "SEMICOLON");
    } else if (c == '\'') {
      tapKey(machine, "APOSTROPHE");
    } else if (c == '[') {
      tapKey(machine, "LEFTBRACKET");
    } else if (c == ']') {
      tapKey(machine, "RIGHTBRACKET");
    } else if (c == '!') {
      tapKey(machine, "1", true);
    } else if (c == '?') {
      tapKey(machine, "SLASH", true);
    } else if (c == ':') {
      tapKey(machine, "SEMICOLON", true);
    }
  }
}

bool Script::load(const std::string &path, std::string &error) {
  std::ifstream in(path);
  if (!in.is_open()) {
    error = "could not open script: " + path;
    return false;
  }

  events_.clear();
  cursor_ = 0;

  std::string line;
  size_t line_no = 0;
  while (std::getline(in, line)) {
    line_no++;
    line = trim(line);
    if (line.empty() || line[0] == '#') continue;

    std::istringstream iss(line);
    std::string at_word;
    std::string at_token;
    uint64_t at = 0;
    std::string kind;
    if (!(iss >> at_word >> at_token >> kind) || at_word != "at" ||
        !parseTickToken(at_token, at)) {
      error = "script line " + std::to_string(line_no) + ": expected 'at <tick> <event>'";
      return false;
    }

    ScriptEvent ev;
    ev.at = at;
    if (kind == "key") {
      std::string action;
      if (!(iss >> ev.key >> action) || !parseBoolAction(action, ev.pressed)) {
        error = "script line " + std::to_string(line_no) + ": expected key <name> <down|up>";
        return false;
      }
      ev.kind = ScriptEvent::Kind::Key;
    } else if (kind == "mouse") {
      unsigned buttons = 0;
      if (!(iss >> ev.dx >> ev.dy >> buttons)) {
        error = "script line " + std::to_string(line_no) + ": expected mouse <dx> <dy> <buttons>";
        return false;
      }
      ev.buttons = static_cast<uint8_t>(buttons & 0x07u);
      ev.kind = ScriptEvent::Kind::Mouse;
    } else if (kind == "move") {
      std::string during;
      std::string duration_token;
      unsigned buttons = 0;
      if (!(iss >> ev.dx >> ev.dy >> during >> duration_token) ||
          (during != "during" && during != "over") ||
          !parseTickToken(duration_token, ev.duration)) {
        error = "script line " + std::to_string(line_no) + ": expected move <dx> <dy> during <duration> [buttons]";
        return false;
      }
      if (iss >> buttons) ev.buttons = static_cast<uint8_t>(buttons & 0x07u);
      ev.kind = ScriptEvent::Kind::Mouse;
      uint64_t steps = ev.duration == 0 ? 1 : ev.duration;
      int emitted_x = 0;
      int emitted_y = 0;
      for (uint64_t i = 0; i < steps; i++) {
        int64_t step_index = static_cast<int64_t>(i + 1);
        int64_t step_count = static_cast<int64_t>(steps);
        int target_x = static_cast<int>((step_index * static_cast<int64_t>(ev.dx)) / step_count);
        int target_y = static_cast<int>((step_index * static_cast<int64_t>(ev.dy)) / step_count);
        ScriptEvent step = ev;
        step.at = at + i;
        step.dx = target_x - emitted_x;
        step.dy = target_y - emitted_y;
        emitted_x = target_x;
        emitted_y = target_y;
        events_.push_back(step);
      }
      continue;
    } else if (kind == "text") {
      std::string rest;
      std::getline(iss, rest);
      ev.text = trim(rest);
      ev.kind = ScriptEvent::Kind::Text;
    } else if (kind == "caption" || kind == "note" || kind == "annotate") {
      std::string first_token;
      std::string duration_token;
      if (!(iss >> first_token)) {
        error = "script line " + std::to_string(line_no) + ": expected caption [top|bottom] <duration> <text>";
        return false;
      }
      if (isCaptionPosition(first_token)) {
        ev.caption_position = first_token;
        if (!(iss >> duration_token) || !parseTickToken(duration_token, ev.duration)) {
          error = "script line " + std::to_string(line_no) + ": expected caption [top|bottom] <duration> <text>";
          return false;
        }
      } else {
        duration_token = first_token;
        if (!parseTickToken(duration_token, ev.duration)) {
          error = "script line " + std::to_string(line_no) + ": expected caption [top|bottom] <duration> <text>";
          return false;
        }
      }
      std::string rest;
      std::getline(iss, rest);
      rest = trim(rest);
      if (!rest.empty()) {
        size_t split = rest.find(' ');
        std::string maybe_position = split == std::string::npos ? rest : rest.substr(0, split);
        if (isCaptionPosition(maybe_position)) {
          ev.caption_position = maybe_position;
          rest = split == std::string::npos ? "" : trim(rest.substr(split + 1));
        }
      }
      if (rest.empty()) {
        error = "script line " + std::to_string(line_no) + ": expected caption text";
        return false;
      }
      ev.text = rest;
      ev.kind = ScriptEvent::Kind::Caption;
    } else if (kind == "capture") {
      std::string state;
      if (!(iss >> state)) {
        error = "script line " + std::to_string(line_no) + ": expected capture <in|out>";
        return false;
      }
      state = upper(state);
      if (state == "IN" || state == "ON") {
        ev.capture_enabled = true;
      } else if (state == "OUT" || state == "OFF") {
        ev.capture_enabled = false;
      } else {
        error = "script line " + std::to_string(line_no) + ": expected capture <in|out>";
        return false;
      }
      ev.kind = ScriptEvent::Kind::Capture;
    } else if (kind == "in" || kind == "out") {
      ev.capture_enabled = kind == "in";
      ev.kind = ScriptEvent::Kind::Capture;
    } else if (kind == "rtc") {
      std::string rest;
      std::getline(iss, rest);
      ev.text = trim(rest);
      ev.kind = ScriptEvent::Kind::Rtc;
    } else if (kind == "tick") {
      ev.kind = ScriptEvent::Kind::Tick;
    } else {
      error = "script line " + std::to_string(line_no) + ": unknown event kind";
      return false;
    }

    events_.push_back(ev);
  }

  std::sort(events_.begin(), events_.end(), [](const ScriptEvent &a, const ScriptEvent &b) {
    return a.at < b.at;
  });
  return true;
}

std::vector<ScriptEvent> Script::takeDue(uint64_t tick) {
  std::vector<ScriptEvent> due;
  while (cursor_ < events_.size() && events_[cursor_].at <= tick) {
    due.push_back(events_[cursor_++]);
  }
  return due;
}

void Script::injectDue(Machine &machine, uint64_t tick) {
  for (const ScriptEvent &ev : takeDue(tick)) {
    switch (ev.kind) {
    case ScriptEvent::Kind::Key:
      machine.injectKey(ev.key, ev.pressed);
      break;
    case ScriptEvent::Kind::Mouse:
      machine.injectMouse(ev.dx, ev.dy, ev.buttons);
      break;
    case ScriptEvent::Kind::Text:
      injectText(machine, ev.text);
      break;
    case ScriptEvent::Kind::Rtc:
      machine.rtc().setIsoTime(ev.text);
      break;
    case ScriptEvent::Kind::Tick:
      break;
    case ScriptEvent::Kind::Caption:
    case ScriptEvent::Kind::Capture:
      break;
    }
  }
}

uint64_t Script::nextTick() const {
  if (cursor_ >= events_.size()) return UINT64_MAX;
  return events_[cursor_].at;
}

} // namespace lcom
