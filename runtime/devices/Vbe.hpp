#ifndef MACHINE_LAB_VBE_HPP
#define MACHINE_LAB_VBE_HPP

#include <cstddef>
#include <cstdint>
#include <string>
#include <vector>

namespace lcom {

struct VbeModeInfo {
  uint16_t mode = 0;
  uint16_t width = 0;
  uint16_t height = 0;
  uint8_t bpp = 0;
  uint8_t bytes_per_pixel = 0;
  uint32_t pitch = 0;
  uint64_t framebuffer_phys = 0;
  uint64_t framebuffer_size = 0;
};

class Vbe {
public:
  Vbe();

  bool modeInfo(uint16_t mode, VbeModeInfo &out) const;
  bool setMode(uint16_t mode);
  const VbeModeInfo &currentMode() const { return current_; }
  bool graphicsMode() const { return graphics_mode_; }

  uint64_t framebufferPhys() const { return framebuffer_phys_; }
  size_t framebufferSize() const { return framebuffer_.size(); }
  std::vector<uint8_t> &framebuffer() { return framebuffer_; }
  const std::vector<uint8_t> &framebuffer() const { return framebuffer_; }
  bool ownsRange(uint64_t phys, uint64_t length) const;
  bool dumpPpm(const std::string &path, const std::string &caption = "",
               const std::string &caption_position = "bottom") const;

private:
  uint64_t framebuffer_phys_ = 0xE0000000ull;
  VbeModeInfo current_{};
  bool graphics_mode_ = false;
  std::vector<uint8_t> framebuffer_;
};

} // namespace lcom

#endif
