#include "Script.hpp"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>

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
    uint64_t at = 0;
    std::string kind;
    if (!(iss >> at_word >> at >> kind) || at_word != "at") {
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
    } else if (kind == "text") {
      std::string rest;
      std::getline(iss, rest);
      ev.text = trim(rest);
      ev.kind = ScriptEvent::Kind::Text;
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

void Script::injectDue(Machine &machine, uint64_t tick) {
  while (cursor_ < events_.size() && events_[cursor_].at <= tick) {
    const ScriptEvent &ev = events_[cursor_++];
    switch (ev.kind) {
    case ScriptEvent::Kind::Key:
      machine.injectKey(ev.key, ev.pressed);
      break;
    case ScriptEvent::Kind::Mouse:
      machine.injectMouse(ev.dx, ev.dy, ev.buttons);
      break;
    case ScriptEvent::Kind::Text:
      for (char c : ev.text) {
        if (c >= 'a' && c <= 'z') {
          std::string key(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
          machine.injectKey(key, true);
          machine.injectKey(key, false);
        } else if (c >= 'A' && c <= 'Z') {
          std::string key(1, c);
          machine.injectKey(key, true);
          machine.injectKey(key, false);
        } else if (c == ' ') {
          machine.injectKey("SPACE", true);
          machine.injectKey("SPACE", false);
        }
      }
      break;
    case ScriptEvent::Kind::Rtc:
      machine.rtc().setIsoTime(ev.text);
      break;
    case ScriptEvent::Kind::Tick:
      break;
    }
  }
}

uint64_t Script::nextTick() const {
  if (cursor_ >= events_.size()) return UINT64_MAX;
  return events_[cursor_].at;
}

} // namespace lcom
