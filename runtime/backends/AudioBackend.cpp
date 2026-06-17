#include "AudioBackend.hpp"

#include <algorithm>
#include <cstdio>
#include <cstring>
#include <utility>

namespace lcom {

static void writeU16(FILE *f, uint16_t value) {
  uint8_t bytes[2] = {static_cast<uint8_t>(value & 0xFFu), static_cast<uint8_t>(value >> 8)};
  std::fwrite(bytes, 1, sizeof(bytes), f);
}

static void writeU32(FILE *f, uint32_t value) {
  uint8_t bytes[4] = {
      static_cast<uint8_t>(value & 0xFFu),
      static_cast<uint8_t>((value >> 8) & 0xFFu),
      static_cast<uint8_t>((value >> 16) & 0xFFu),
      static_cast<uint8_t>((value >> 24) & 0xFFu),
  };
  std::fwrite(bytes, 1, sizeof(bytes), f);
}

bool WavAudioBackend::playPcm16(const int16_t *samples,
                                size_t frame_count,
                                uint32_t sample_rate,
                                uint8_t channels,
                                std::string &error) {
  if (samples == nullptr || frame_count == 0 || channels == 0) {
    error = "empty PCM buffer";
    return false;
  }

  FILE *f = std::fopen(path_.c_str(), "wb");
  if (f == nullptr) {
    error = "could not open WAV output: " + path_;
    return false;
  }

  uint32_t data_bytes = static_cast<uint32_t>(frame_count * channels * sizeof(int16_t));
  uint32_t byte_rate = sample_rate * channels * sizeof(int16_t);
  uint16_t block_align = static_cast<uint16_t>(channels * sizeof(int16_t));

  std::fwrite("RIFF", 1, 4, f);
  writeU32(f, 36u + data_bytes);
  std::fwrite("WAVE", 1, 4, f);
  std::fwrite("fmt ", 1, 4, f);
  writeU32(f, 16);
  writeU16(f, 1);
  writeU16(f, channels);
  writeU32(f, sample_rate);
  writeU32(f, byte_rate);
  writeU16(f, block_align);
  writeU16(f, 16);
  std::fwrite("data", 1, 4, f);
  writeU32(f, data_bytes);
  std::fwrite(samples, sizeof(int16_t), frame_count * channels, f);
  std::fclose(f);
  return true;
}

} // namespace lcom
