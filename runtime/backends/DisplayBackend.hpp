#ifndef MACHINE_LAB_DISPLAY_BACKEND_HPP
#define MACHINE_LAB_DISPLAY_BACKEND_HPP

#include "../core/Machine.hpp"

namespace lcom {

class DisplayBackend {
public:
  virtual ~DisplayBackend() = default;
  virtual bool start(Machine &) { return true; }
  virtual void pump(Machine &) {}
  virtual void consoleWrite(const char *, size_t) {}
  virtual void present(Machine &) {}
};

} // namespace lcom

#endif
