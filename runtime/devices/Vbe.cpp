#include "Vbe.hpp"

#include <lcom/vbe.h>

#include <algorithm>
#include <cstdio>

namespace lcom {

Vbe::Vbe() {
  setMode(LCOM_VBE_MODE_800_600_24);
  graphics_mode_ = false;
}

bool Vbe::modeInfo(uint16_t mode, VbeModeInfo &out) const {
  out = {};
  out.mode = mode;
  out.framebuffer_phys = framebuffer_phys_;

  switch (mode) {
  case LCOM_VBE_MODE_1024_768_8:
    out.width = 1024;
    out.height = 768;
    out.bpp = 8;
    out.bytes_per_pixel = 1;
    break;
  case LCOM_VBE_MODE_640_480_15:
    out.width = 640;
    out.height = 480;
    out.bpp = 15;
    out.bytes_per_pixel = 2;
    break;
  case LCOM_VBE_MODE_800_600_24:
    out.width = 800;
    out.height = 600;
    out.bpp = 24;
    out.bytes_per_pixel = 3;
    break;
  case LCOM_VBE_MODE_1280_1024_32:
    out.width = 1280;
    out.height = 1024;
    out.bpp = 32;
    out.bytes_per_pixel = 4;
    break;
  default:
    return false;
  }

  out.pitch = static_cast<uint32_t>(out.width * out.bytes_per_pixel);
  out.framebuffer_size = static_cast<uint64_t>(out.pitch) * out.height;
  return true;
}

bool Vbe::setMode(uint16_t mode) {
  VbeModeInfo info;
  if (!modeInfo(mode, info)) return false;
  current_ = info;
  framebuffer_.assign(static_cast<size_t>(info.framebuffer_size), 0);
  graphics_mode_ = true;
  return true;
}

bool Vbe::ownsRange(uint64_t phys, uint64_t length) const {
  if (length == 0) return false;
  uint64_t start = framebuffer_phys_;
  uint64_t end = start + framebuffer_.size();
  return phys >= start && phys + length <= end;
}

bool Vbe::dumpPpm(const std::string &path) const {
  if (!graphics_mode_ || framebuffer_.empty()) return false;
  FILE *f = std::fopen(path.c_str(), "wb");
  if (f == nullptr) return false;
  std::fprintf(f, "P6\n%u %u\n255\n", current_.width, current_.height);
  for (uint16_t y = 0; y < current_.height; y++) {
    const uint8_t *row = framebuffer_.data() + static_cast<size_t>(y) * current_.pitch;
    for (uint16_t x = 0; x < current_.width; x++) {
      const uint8_t *px = row + static_cast<size_t>(x) * current_.bytes_per_pixel;
      uint8_t rgb[3] = {0, 0, 0};
      if (current_.bytes_per_pixel == 1) {
        rgb[0] = rgb[1] = rgb[2] = px[0];
      } else if (current_.bytes_per_pixel == 2) {
        uint16_t v = static_cast<uint16_t>(px[0] | (px[1] << 8));
        rgb[0] = static_cast<uint8_t>(((v >> 10) & 0x1F) * 255 / 31);
        rgb[1] = static_cast<uint8_t>(((v >> 5) & 0x1F) * 255 / 31);
        rgb[2] = static_cast<uint8_t>((v & 0x1F) * 255 / 31);
      } else {
        rgb[0] = px[2];
        rgb[1] = px[1];
        rgb[2] = px[0];
      }
      std::fwrite(rgb, 1, sizeof(rgb), f);
    }
  }
  std::fclose(f);
  return true;
}

} // namespace lcom
