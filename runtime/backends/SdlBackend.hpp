#ifndef MACHINE_LAB_SDL_BACKEND_HPP
#define MACHINE_LAB_SDL_BACKEND_HPP

#include "DisplayBackend.hpp"

#include <memory>
#include <string>

namespace lcom {

struct SdlBackendOptions {
  bool fullscreen = false;
  bool integer_scale = false;
  bool guest_input = true;
  int scale = 1;
  std::string title = "LCOM Display";
};

#if defined(MACHINE_LAB_WITH_SDL)
std::unique_ptr<DisplayBackend> createSdlBackend(const SdlBackendOptions &options);
#endif

} // namespace lcom

#endif
