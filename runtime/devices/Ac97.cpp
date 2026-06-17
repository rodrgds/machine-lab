#include "Ac97.hpp"

#include <lcom/ac97.h>

#include <algorithm>
#include <cstring>

namespace lcom {

Ac97::Ac97() : pcm_(2u * 1024u * 1024u, 0) {
  nam_[AC97_EXT_AUDIO_ID] = 0x01;
  nam_[AC97_EXT_AUDIO_CTRL] = 0x01;
  nam_[AC97_PCM_FRONT_DAC_RATE] = static_cast<uint8_t>(sample_rate_ & 0xFFu);
  nam_[AC97_PCM_FRONT_DAC_RATE + 1] = static_cast<uint8_t>(sample_rate_ >> 8);
  bm_[AC97_PO_SR] = AC97_PO_SR_DCH;
}

bool Ac97::handles(uint16_t port) const {
  return (port >= AC97_NAM_BASE && port < AC97_NAM_BASE + nam_.size()) ||
         (port >= AC97_BM_BASE && port < AC97_BM_BASE + bm_.size());
}

bool Ac97::read8(uint16_t port, uint8_t &value) {
  if (port >= AC97_NAM_BASE && port < AC97_NAM_BASE + nam_.size()) {
    value = readNamed(static_cast<uint16_t>(port - AC97_NAM_BASE));
    return true;
  }
  if (port >= AC97_BM_BASE && port < AC97_BM_BASE + bm_.size()) {
    value = readBusMaster(static_cast<uint16_t>(port - AC97_BM_BASE));
    return true;
  }
  return false;
}

bool Ac97::write8(uint16_t port, uint8_t value) {
  if (port >= AC97_NAM_BASE && port < AC97_NAM_BASE + nam_.size()) {
    writeNamed(static_cast<uint16_t>(port - AC97_NAM_BASE), value);
    return true;
  }
  if (port >= AC97_BM_BASE && port < AC97_BM_BASE + bm_.size()) {
    writeBusMaster(static_cast<uint16_t>(port - AC97_BM_BASE), value);
    return true;
  }
  return false;
}

bool Ac97::ownsRange(uint64_t phys, uint64_t length) const {
  if (length == 0) return false;
  return phys >= pcm_phys_ && phys + length <= pcm_phys_ + pcm_.size();
}

bool Ac97::play(size_t byte_count, uint32_t sample_rate, uint8_t channels) {
  if (byte_count == 0 || byte_count > pcm_.size()) return false;
  if (channels == 0 || channels > 2) return false;
  sample_rate_ = sample_rate == 0 ? sample_rate_ : sample_rate;
  channels_ = channels;
  play_byte_count_ = byte_count;
  playing_ = true;
  bm_[AC97_PO_SR] = AC97_PO_SR_BCIS;
  bm_[AC97_PO_CR] |= AC97_PO_CR_RUN;
  nam_[AC97_PCM_FRONT_DAC_RATE] = static_cast<uint8_t>(sample_rate_ & 0xFFu);
  nam_[AC97_PCM_FRONT_DAC_RATE + 1] = static_cast<uint8_t>(sample_rate_ >> 8);
  return true;
}

void Ac97::stop() {
  playing_ = false;
  play_byte_count_ = 0;
  bm_[AC97_PO_CR] &= static_cast<uint8_t>(~AC97_PO_CR_RUN);
  bm_[AC97_PO_SR] = AC97_PO_SR_DCH;
}

uint8_t Ac97::readNamed(uint16_t offset) const {
  return offset < nam_.size() ? nam_[offset] : 0;
}

void Ac97::writeNamed(uint16_t offset, uint8_t value) {
  if (offset >= nam_.size()) return;
  nam_[offset] = value;
  if (offset == AC97_RESET) {
    sample_rate_ = AC97_DEFAULT_SAMPLE_RATE;
    channels_ = AC97_DEFAULT_CHANNELS;
    playing_ = false;
    play_byte_count_ = 0;
  }
  if (offset == AC97_PCM_FRONT_DAC_RATE || offset == AC97_PCM_FRONT_DAC_RATE + 1) {
    sample_rate_ = static_cast<uint32_t>(nam_[AC97_PCM_FRONT_DAC_RATE] |
                                        (nam_[AC97_PCM_FRONT_DAC_RATE + 1] << 8));
    if (sample_rate_ == 0) sample_rate_ = AC97_DEFAULT_SAMPLE_RATE;
  }
}

uint8_t Ac97::readBusMaster(uint16_t offset) const {
  return offset < bm_.size() ? bm_[offset] : 0;
}

void Ac97::writeBusMaster(uint16_t offset, uint8_t value) {
  if (offset >= bm_.size()) return;
  bm_[offset] = value;
  if (offset == AC97_PO_PICB || offset == AC97_PO_PICB + 1) {
    play_byte_count_ = static_cast<size_t>(bm_[AC97_PO_PICB] |
                                           (bm_[AC97_PO_PICB + 1] << 8));
    if (play_byte_count_ > pcm_.size()) play_byte_count_ = pcm_.size();
  }
  if (offset == AC97_PO_CR) {
    if (value & AC97_PO_CR_RESET) stop();
    if ((value & AC97_PO_CR_RUN) != 0) {
      if (play_byte_count_ == 0 || play_byte_count_ > pcm_.size()) {
        play_byte_count_ = pcm_.size();
      }
      playing_ = true;
      bm_[AC97_PO_SR] = AC97_PO_SR_BCIS;
    } else {
      playing_ = false;
      bm_[AC97_PO_SR] = AC97_PO_SR_DCH;
    }
  }
}

} // namespace lcom
