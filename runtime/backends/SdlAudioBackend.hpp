#ifndef LCOM_NG_SDL_AUDIO_BACKEND_HPP
#define LCOM_NG_SDL_AUDIO_BACKEND_HPP

#include "AudioBackend.hpp"

#include <memory>

namespace lcom {

#if defined(LCOM_WITH_SDL)
std::unique_ptr<AudioBackend> createSdlAudioBackend();
#endif

} // namespace lcom

#endif
