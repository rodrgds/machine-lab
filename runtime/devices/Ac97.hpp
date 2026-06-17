#ifndef LCOM_NG_AC97_DEVICE_HPP
#define LCOM_NG_AC97_DEVICE_HPP

#include "../core/PortDevice.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

namespace lcom {

class Ac97 : public PortDevice {
public:
  Ac97();

  bool handles(uint16_t port) const override;
  bool read8(uint16_t port, uint8_t &value) override;
  bool write8(uint16_t port, uint8_t value) override;

  uint64_t bufferPhys() const { return pcm_phys_; }
  size_t bufferSize() const { return pcm_.size(); }
  bool ownsRange(uint64_t phys, uint64_t length) const;
  std::vector<uint8_t> &pcm() { return pcm_; }
  const std::vector<uint8_t> &pcm() const { return pcm_; }

  uint32_t sampleRate() const { return sample_rate_; }
  uint8_t channels() const { return channels_; }
  bool playing() const { return playing_; }

  bool play(size_t byte_count, uint32_t sample_rate, uint8_t channels);
  void stop();
  size_t playByteCount() const { return play_byte_count_; }

private:
  uint8_t readNamed(uint16_t offset) const;
  void writeNamed(uint16_t offset, uint8_t value);
  uint8_t readBusMaster(uint16_t offset) const;
  void writeBusMaster(uint16_t offset, uint8_t value);

  uint64_t pcm_phys_ = 0xD0000000ull;
  std::vector<uint8_t> pcm_;
  std::array<uint8_t, 0x80> nam_{};
  std::array<uint8_t, 0x40> bm_{};
  uint32_t sample_rate_ = 48000;
  uint8_t channels_ = 2;
  bool playing_ = false;
  size_t play_byte_count_ = 0;
};

} // namespace lcom

#endif
