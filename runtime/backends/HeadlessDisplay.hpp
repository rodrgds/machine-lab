#ifndef MACHINE_LAB_HEADLESS_DISPLAY_HPP
#define MACHINE_LAB_HEADLESS_DISPLAY_HPP

#include "DisplayBackend.hpp"

namespace lcom {

class HeadlessDisplay final : public DisplayBackend {
public:
  bool start(Machine &) override { return true; }
  void pump(Machine &) override {}
  void present(Machine &) override {}
};

} // namespace lcom

#endif
