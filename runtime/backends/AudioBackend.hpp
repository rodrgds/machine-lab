#ifndef LCOM_NG_AUDIO_BACKEND_HPP
#define LCOM_NG_AUDIO_BACKEND_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <utility>

namespace lcom {

class AudioBackend {
public:
  virtual ~AudioBackend() = default;
  virtual bool start(std::string &error) {
    (void)error;
    return true;
  }
  virtual bool playPcm16(const int16_t *samples,
                         size_t frame_count,
                         uint32_t sample_rate,
                         uint8_t channels,
                         std::string &error) = 0;
  virtual void stop() {}
};

class NullAudioBackend final : public AudioBackend {
public:
  bool playPcm16(const int16_t *, size_t, uint32_t, uint8_t, std::string &) override {
    return true;
  }
};

class WavAudioBackend final : public AudioBackend {
public:
  explicit WavAudioBackend(std::string path) : path_(std::move(path)) {}
  bool playPcm16(const int16_t *samples,
                 size_t frame_count,
                 uint32_t sample_rate,
                 uint8_t channels,
                 std::string &error) override;

private:
  std::string path_;
};

} // namespace lcom

#endif
