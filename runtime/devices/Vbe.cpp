#include "Vbe.hpp"

#include <lcom/vbe.h>

#include <algorithm>
#include <cctype>
#include <cstdio>
#include <cstring>

namespace lcom {

namespace {

const char *glyphRows(char c) {
  switch (std::toupper(static_cast<unsigned char>(c))) {
  case 'A': return "01110100011000111111100011000110001";
  case 'B': return "11110100011000111110100011000111110";
  case 'C': return "01111100001000010000100001000001111";
  case 'D': return "11110100011000110001100011000111110";
  case 'E': return "11111100001111010000100001000011111";
  case 'F': return "11111100001111010000100001000010000";
  case 'G': return "01111100001000010111100011000101111";
  case 'H': return "10001100011000111111100011000110001";
  case 'I': return "11111001000010000100001000010011111";
  case 'J': return "00111000100001000010000101001001100";
  case 'K': return "10001100101010011000101001001010001";
  case 'L': return "10000100001000010000100001000011111";
  case 'M': return "10001110111010110101100011000110001";
  case 'N': return "10001110011010110011100011000110001";
  case 'O': return "01110100011000110001100011000101110";
  case 'P': return "11110100011000111110100001000010000";
  case 'Q': return "01110100011000110001101011001001101";
  case 'R': return "11110100011000111110101001001010001";
  case 'S': return "01111100001000001110000010000111110";
  case 'T': return "11111001000010000100001000010000100";
  case 'U': return "10001100011000110001100011000101110";
  case 'V': return "10001100011000110001100010101000100";
  case 'W': return "10001100011000110101101011101110001";
  case 'X': return "10001100010101000100010101000110001";
  case 'Y': return "10001100010101000100001000010000100";
  case 'Z': return "11111000010001000100010001000011111";
  case '0': return "01110100011001110101110011000101110";
  case '1': return "00100011000010000100001000010001110";
  case '2': return "01110100010000100010001000100011111";
  case '3': return "11110000010000101110000010000111110";
  case '4': return "00010001100101010010111110001000010";
  case '5': return "11111100001111000001000011000101110";
  case '6': return "00110010001000011110100011000101110";
  case '7': return "11111000010001000100010000100001000";
  case '8': return "01110100011000101110100011000101110";
  case '9': return "01110100011000101111000010001001100";
  case ':': return "00000001000010000000001000010000000";
  case '.': return "00000000000000000000000000010000100";
  case '-': return "00000000000000011111000000000000000";
  case '/': return "00001000100001000100010001000010000";
  case '!': return "00100001000010000100001000000000100";
  case '?': return "01110100010000100010001000000000100";
  default: return "00000000000000000000000000000000000";
  }
}

bool captionPixel(const std::string &caption, const std::string &position,
                  uint16_t width, uint16_t height,
                  uint16_t x, uint16_t y, uint8_t rgb[3]) {
  if (caption.empty() || width < 160 || height < 96) return false;
  const int scale = 3;
  const int char_w = 6 * scale;
  const int text_h = 7 * scale;
  const int max_chars = std::max(1, (static_cast<int>(width) - 96) / char_w);
  int text_len = static_cast<int>(std::min(caption.size(), static_cast<size_t>(max_chars)));
  int box_w = text_len * char_w + 48;
  int box_h = text_h + 28;
  int box_x = (static_cast<int>(width) - box_w) / 2;
  int box_y = position == "top" ? 24 : static_cast<int>(height) - box_h - 24;

  if (x < box_x || x >= box_x + box_w || y < box_y || y >= box_y + box_h) return false;

  rgb[0] = 18;
  rgb[1] = 24;
  rgb[2] = 32;

  int text_x = box_x + 24;
  int text_y = box_y + 14;
  int lx = static_cast<int>(x) - text_x;
  int ly = static_cast<int>(y) - text_y;
  if (lx < 0 || ly < 0 || ly >= text_h) return true;
  int char_index = lx / char_w;
  if (char_index < 0 || char_index >= text_len) return true;
  char c = caption[static_cast<size_t>(char_index)];
  if (c == ' ') return true;
  int in_char_x = (lx % char_w) / scale;
  int in_char_y = ly / scale;
  if (in_char_x >= 5) return true;
  const char *rows = glyphRows(c);
  if (rows[in_char_y * 5 + in_char_x] == '1') {
    rgb[0] = 236;
    rgb[1] = 246;
    rgb[2] = 255;
  }
  return true;
}

} // namespace

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
  return phys >= start && phys <= end && length <= end - phys;
}

bool Vbe::dumpPpm(const std::string &path, const std::string &caption,
                  const std::string &caption_position) const {
  if (!graphics_mode_ || framebuffer_.empty()) return false;
  FILE *f = std::fopen(path.c_str(), "wb");
  if (f == nullptr) return false;
  if (std::fprintf(f, "P6\n%u %u\n255\n", current_.width, current_.height) < 0) {
    std::fclose(f);
    return false;
  }
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
      captionPixel(caption, caption_position, current_.width, current_.height, x, y, rgb);
      if (std::fwrite(rgb, 1, sizeof(rgb), f) != sizeof(rgb)) {
        std::fclose(f);
        return false;
      }
    }
  }
  return std::fclose(f) == 0;
}

} // namespace lcom
