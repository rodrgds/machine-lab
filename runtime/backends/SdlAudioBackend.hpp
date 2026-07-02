#ifndef MACHINE_LAB_SDL_AUDIO_BACKEND_HPP
#define MACHINE_LAB_SDL_AUDIO_BACKEND_HPP

#include "AudioBackend.hpp"

#include <memory>

namespace lcom {

#if defined(MACHINE_LAB_WITH_SDL)
std::unique_ptr<AudioBackend> createSdlAudioBackend();
#endif

} // namespace lcom

#endif
