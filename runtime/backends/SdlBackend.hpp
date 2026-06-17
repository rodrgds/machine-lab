#ifndef LCOM_NG_SDL_BACKEND_HPP
#define LCOM_NG_SDL_BACKEND_HPP

#include "DisplayBackend.hpp"

#include <memory>
#include <string>

namespace lcom {

struct SdlBackendOptions {
  bool fullscreen = false;
  bool integer_scale = false;
  int scale = 1;
  std::string title = "LCOM Display";
};

#if defined(LCOM_WITH_SDL)
std::unique_ptr<DisplayBackend> createSdlBackend(const SdlBackendOptions &options);
#endif

} // namespace lcom

#endif
