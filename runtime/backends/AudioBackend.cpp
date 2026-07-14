#include "AudioBackend.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <limits>
#include <utility>

namespace lcom {

static bool writeU16(FILE *f, uint16_t value) {
  uint8_t bytes[2] = {static_cast<uint8_t>(value & 0xFFu), static_cast<uint8_t>(value >> 8)};
  return std::fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes);
}

static bool writeU32(FILE *f, uint32_t value) {
  uint8_t bytes[4] = {
      static_cast<uint8_t>(value & 0xFFu),
      static_cast<uint8_t>((value >> 8) & 0xFFu),
      static_cast<uint8_t>((value >> 16) & 0xFFu),
      static_cast<uint8_t>((value >> 24) & 0xFFu),
  };
  return std::fwrite(bytes, 1, sizeof(bytes), f) == sizeof(bytes);
}

WavAudioBackend::~WavAudioBackend() { close(nullptr); }

bool WavAudioBackend::open(uint32_t sample_rate, uint8_t channels, std::string &error) {
  file_ = std::fopen(path_.c_str(), "wb+");
  if (file_ == nullptr) {
    error = "could not open WAV output: " + path_;
    return false;
  }
  sample_rate_ = sample_rate;
  channels_ = channels;
  return updateHeader(error);
}

bool WavAudioBackend::updateHeader(std::string &error) {
  if (file_ == nullptr || std::fseek(file_, 0, SEEK_SET) != 0) {
    error = "could not seek WAV output: " + path_;
    return false;
  }

  const bool wrote_header =
      std::fwrite("RIFF", 1, 4, file_) == 4 &&
      writeU32(file_, 36u + data_bytes_) &&
      std::fwrite("WAVEfmt ", 1, 8, file_) == 8 &&
      writeU32(file_, 16) && writeU16(file_, 1) &&
      writeU16(file_, channels_) && writeU32(file_, sample_rate_) &&
      writeU32(file_, sample_rate_ * channels_ * sizeof(int16_t)) &&
      writeU16(file_, static_cast<uint16_t>(channels_ * sizeof(int16_t))) &&
      writeU16(file_, 16) &&
      std::fwrite("data", 1, 4, file_) == 4 && writeU32(file_, data_bytes_);
  if (!wrote_header || std::fflush(file_) != 0 ||
      std::fseek(file_, 0, SEEK_END) != 0) {
    error = "could not update WAV output: " + path_;
    return false;
  }
  return true;
}

bool WavAudioBackend::close(std::string *error) {
  if (file_ == nullptr) return true;
  std::string header_error;
  bool ok = updateHeader(header_error);
  if (std::fclose(file_) != 0) {
    ok = false;
    if (header_error.empty()) header_error = "could not close WAV output: " + path_;
  }
  file_ = nullptr;
  if (!ok && error != nullptr) *error = header_error;
  return ok;
}

void WavAudioBackend::stop() {
  if (file_ != nullptr) std::fflush(file_);
}

bool WavAudioBackend::playPcm16(const int16_t *samples,
                                size_t frame_count,
                                uint32_t sample_rate,
                                uint8_t channels,
                                std::string &error) {
  if (samples == nullptr || frame_count == 0 || sample_rate == 0 ||
      channels == 0 || channels > 2) {
    error = "empty PCM buffer";
    return false;
  }

  if (file_ != nullptr && (sample_rate_ != sample_rate || channels_ != channels)) {
    error = "PCM format changed during WAV capture";
    return false;
  }
  if (file_ == nullptr && !open(sample_rate, channels, error)) return false;

  if (frame_count > std::numeric_limits<uint64_t>::max() / channels) {
    error = "WAV sample count overflow";
    return false;
  }
  const uint64_t sample_count = static_cast<uint64_t>(frame_count) * channels;
  const uint64_t byte_count = sample_count * sizeof(int16_t);
  constexpr uint64_t kMaxWavDataBytes = std::numeric_limits<uint32_t>::max() - 36u;
  if (sample_count > std::numeric_limits<size_t>::max() ||
      byte_count > kMaxWavDataBytes - data_bytes_) {
    error = "WAV output exceeds the 4 GiB RIFF limit";
    return false;
  }
  if (std::fwrite(samples, sizeof(int16_t), static_cast<size_t>(sample_count), file_) !=
      static_cast<size_t>(sample_count)) {
    error = "could not write WAV output: " + path_;
    return false;
  }
  data_bytes_ += static_cast<uint32_t>(byte_count);
  return updateHeader(error);
}

} // namespace lcom
