#if defined(LCOM_WITH_SDL)

#include "SdlAudioBackend.hpp"

#include <SDL3/SDL.h>

#include <cstdio>

namespace lcom {

class SdlAudioBackend final : public AudioBackend {
public:
  bool start(std::string &error) override {
    if (SDL_WasInit(SDL_INIT_AUDIO) == 0 && !SDL_InitSubSystem(SDL_INIT_AUDIO)) {
      error = SDL_GetError();
      return false;
    }
    return true;
  }

  bool playPcm16(const int16_t *samples,
                 size_t frame_count,
                 uint32_t sample_rate,
                 uint8_t channels,
                 std::string &error) override {
    if (samples == nullptr || frame_count == 0 || channels == 0) return true;
    if (stream_ == nullptr || sample_rate_ != sample_rate || channels_ != channels) {
      if (stream_ != nullptr) {
        SDL_DestroyAudioStream(stream_);
        stream_ = nullptr;
      }

      SDL_AudioSpec want{};
      want.freq = static_cast<int>(sample_rate);
      want.format = SDL_AUDIO_S16;
      want.channels = channels;

      stream_ = SDL_OpenAudioDeviceStream(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, &want, nullptr, nullptr);
      if (stream_ == nullptr) {
        error = SDL_GetError();
        return false;
      }
      sample_rate_ = sample_rate;
      channels_ = channels;
    }

    int bytes = static_cast<int>(frame_count * channels * sizeof(int16_t));
    if (SDL_GetAudioStreamQueued(stream_) > static_cast<int>(sample_rate * channels * sizeof(int16_t) / 3u)) {
      SDL_ClearAudioStream(stream_);
    }
    if (!SDL_PutAudioStreamData(stream_, samples, bytes)) {
      error = SDL_GetError();
      return false;
    }
    SDL_ResumeAudioStreamDevice(stream_);
    return true;
  }

  void stop() override {
    if (stream_ != nullptr) {
      SDL_DestroyAudioStream(stream_);
      stream_ = nullptr;
    }
  }

  ~SdlAudioBackend() override {
    stop();
  }

private:
  SDL_AudioStream *stream_ = nullptr;
  uint32_t sample_rate_ = 0;
  uint8_t channels_ = 0;
};

std::unique_ptr<AudioBackend> createSdlAudioBackend() {
  return std::unique_ptr<AudioBackend>(new SdlAudioBackend());
}

} // namespace lcom

#endif
