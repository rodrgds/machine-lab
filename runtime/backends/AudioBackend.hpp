#ifndef MACHINE_LAB_AUDIO_BACKEND_HPP
#define MACHINE_LAB_AUDIO_BACKEND_HPP

#include <cstddef>
#include <cstdint>
#include <cstdio>
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
  ~WavAudioBackend() override;
  bool playPcm16(const int16_t *samples,
                 size_t frame_count,
                 uint32_t sample_rate,
                 uint8_t channels,
                 std::string &error) override;
  void stop() override;

private:
  bool open(uint32_t sample_rate, uint8_t channels, std::string &error);
  bool updateHeader(std::string &error);
  bool close(std::string *error);

  std::string path_;
  FILE *file_ = nullptr;
  uint32_t data_bytes_ = 0;
  uint32_t sample_rate_ = 0;
  uint8_t channels_ = 0;
};

} // namespace lcom

#endif
