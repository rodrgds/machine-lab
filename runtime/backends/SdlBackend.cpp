#if defined(LCOM_WITH_SDL)

#include "SdlBackend.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

namespace lcom {

namespace {

bool fileExists(const char *path) {
  if (path == nullptr || path[0] == '\0') return false;
  std::ifstream in(path);
  return in.good();
}

std::string findFontPath() {
  const char *env = std::getenv("LCOM_FONT");
  if (fileExists(env)) return env;

  const char *candidates[] = {
      "/System/Library/Fonts/Menlo.ttc",
      "/System/Library/Fonts/Supplemental/Menlo.ttc",
      "/Library/Fonts/Arial Unicode.ttf",
      "/usr/share/fonts/truetype/dejavu/DejaVuSansMono.ttf",
      "/usr/share/fonts/dejavu/DejaVuSansMono.ttf",
      nullptr,
  };
  for (const char **p = candidates; *p != nullptr; ++p) {
    if (fileExists(*p)) return *p;
  }
  return "";
}

SDL_Color rgba(uint8_t r, uint8_t g, uint8_t b, uint8_t a = 255) {
  SDL_Color c{r, g, b, a};
  return c;
}

constexpr uint64_t kMouseSampleIntervalNs = 1000000000ull / 60ull;
constexpr int kMouseStep = 255;

} // namespace

class SdlBackend final : public DisplayBackend {
public:
  explicit SdlBackend(SdlBackendOptions options) : options_(std::move(options)) {}

  bool start(Machine &machine) override {
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)) {
      std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      return false;
    }
    if (!TTF_Init()) {
      std::fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
      return false;
    }

    const VbeModeInfo &mode = machine.vbe().currentMode();
    logical_width_ = mode.width;
    logical_height_ = mode.height;
    guest_mouse_x_ = logical_width_ / 2;
    guest_mouse_y_ = logical_height_ / 2;

    SDL_WindowFlags flags = SDL_WINDOW_RESIZABLE;
    if (options_.fullscreen) flags |= SDL_WINDOW_FULLSCREEN;
    int scale = options_.scale <= 0 ? 1 : options_.scale;
    window_ = SDL_CreateWindow(options_.title.c_str(),
                               logical_width_ * scale,
                               logical_height_ * scale,
                               flags);
    if (window_ == nullptr) {
      std::fprintf(stderr, "SDL_CreateWindow failed: %s\n", SDL_GetError());
      return false;
    }

    renderer_ = SDL_CreateRenderer(window_, nullptr);
    if (renderer_ == nullptr) {
      renderer_ = SDL_CreateRenderer(window_, "software");
    }
    if (renderer_ == nullptr) {
      std::fprintf(stderr, "SDL_CreateRenderer failed: %s\n", SDL_GetError());
      return false;
    }

    SDL_SetRenderLogicalPresentation(renderer_, logical_width_, logical_height_,
                                     options_.integer_scale
                                         ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
                                         : SDL_LOGICAL_PRESENTATION_LETTERBOX);

    std::string font_path = findFontPath();
    if (!font_path.empty()) {
      font_ = TTF_OpenFont(font_path.c_str(), 14);
      title_font_ = TTF_OpenFont(font_path.c_str(), 15);
    }
    if (font_ == nullptr || title_font_ == nullptr) {
      std::fprintf(stderr, "lcom: could not load a TTF font; set LCOM_FONT for SDL text\n");
    }

    terminal_lines_.push_back("LCOM Terminal");
    terminal_lines_.push_back("Press F3 for device state.");
    SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, false);
    SDL_FlushEvent(SDL_EVENT_MOUSE_MOTION);
    return ensureTexture(machine);
  }

  void pump(Machine &machine) override {
    SDL_Event ev;

    updateMouseCapture(machine);
    while (SDL_PollEvent(&ev)) {
      switch (ev.type) {
      case SDL_EVENT_QUIT:
        should_quit_ = true;
        machine.injectKey("ESC", true);
        machine.injectKey("ESC", false);
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
      case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        setMouseCaptured(false);
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
      case SDL_EVENT_WINDOW_MOUSE_ENTER:
        updateMouseCapture(machine);
        break;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        sampleMouse(machine, true);
        handleKey(machine, ev.key);
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP: {
        int lx = 0;
        int ly = 0;
        logicalPoint(ev.button.x, ev.button.y, lx, ly);
        if (!isChromePoint(lx, ly) && machine.i8042().mouseReporting() && !debug_visible_) {
          updateMouseCapture(machine);
          sampleMouse(machine, true);
          updateHostButton(ev.button.button, ev.button.down);
          machine.injectMouse(0, 0, host_mouse_buttons_);
          last_mouse_sample_ns_ = SDL_GetTicksNS();
        }
        break;
      }
      default:
        break;
      }
    }
    updateMouseCapture(machine);
    sampleMouse(machine, false);
  }

  void consoleWrite(const char *data, size_t size) override {
    for (size_t i = 0; i < size; i++) {
      char c = data[i];
      if (c == '\r') continue;
      if (c == '\n') {
        terminal_lines_.push_back(current_line_);
        current_line_.clear();
      } else {
        current_line_.push_back(c);
      }
    }
    while (terminal_lines_.size() > 1000) terminal_lines_.erase(terminal_lines_.begin());
  }

  void present(Machine &machine) override {
    if (renderer_ == nullptr) return;
    recordFrameTiming();
    if (machine.vbe().graphicsMode()) {
      renderFramebuffer(machine);
    } else {
      renderTerminal();
    }
    renderChrome(machine);
    SDL_RenderPresent(renderer_);
  }

  ~SdlBackend() override {
    setMouseCaptured(false);
    if (texture_ != nullptr) SDL_DestroyTexture(texture_);
    if (font_ != nullptr) TTF_CloseFont(font_);
    if (title_font_ != nullptr) TTF_CloseFont(title_font_);
    if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
    if (window_ != nullptr) SDL_DestroyWindow(window_);
    TTF_Quit();
    SDL_Quit();
  }

private:
  bool ensureTexture(Machine &machine) {
    const VbeModeInfo &mode = machine.vbe().currentMode();
    if (texture_ != nullptr && texture_width_ == mode.width && texture_height_ == mode.height) {
      return true;
    }
    if (texture_ != nullptr) {
      SDL_DestroyTexture(texture_);
      texture_ = nullptr;
    }
    texture_width_ = mode.width;
    texture_height_ = mode.height;
    logical_width_ = mode.width;
    logical_height_ = mode.height;
    SDL_SetRenderLogicalPresentation(renderer_, logical_width_, logical_height_,
                                     options_.integer_scale
                                         ? SDL_LOGICAL_PRESENTATION_INTEGER_SCALE
                                         : SDL_LOGICAL_PRESENTATION_LETTERBOX);
    texture_ = SDL_CreateTexture(renderer_, SDL_PIXELFORMAT_RGB24, SDL_TEXTUREACCESS_STREAMING,
                                 texture_width_, texture_height_);
    if (texture_ == nullptr) {
      std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
      return false;
    }
    return true;
  }

  void handleKey(Machine &machine, const SDL_KeyboardEvent &key) {
    bool down = key.down;
    if (down && key.scancode == SDL_SCANCODE_F3) {
      debug_visible_ = !debug_visible_;
      updateMouseCapture(machine);
      return;
    }
    if (down && key.scancode == SDL_SCANCODE_F11) {
      fullscreen_ = !fullscreen_;
      SDL_SetWindowFullscreen(window_, fullscreen_);
      return;
    }
    if (key.repeat) return;

    switch (key.scancode) {
    case SDL_SCANCODE_ESCAPE:
      machine.injectKey("ESC", down);
      break;
    case SDL_SCANCODE_SPACE:
      machine.injectKey("SPACE", down);
      break;
    case SDL_SCANCODE_RETURN:
      machine.injectKey("ENTER", down);
      break;
    case SDL_SCANCODE_UP:
      machine.injectKey("UP", down);
      break;
    case SDL_SCANCODE_DOWN:
      machine.injectKey("DOWN", down);
      break;
    case SDL_SCANCODE_LEFT:
      machine.injectKey("LEFT", down);
      break;
    case SDL_SCANCODE_RIGHT:
      machine.injectKey("RIGHT", down);
      break;
    default:
      if (key.key >= SDLK_A && key.key <= SDLK_Z) {
        char name[2] = {static_cast<char>('A' + (key.key - SDLK_A)), 0};
        machine.injectKey(name, down);
      } else if (key.key >= SDLK_0 && key.key <= SDLK_9) {
        char name[2] = {static_cast<char>('0' + (key.key - SDLK_0)), 0};
        machine.injectKey(name, down);
      }
      break;
    }
  }

  void renderFramebuffer(Machine &machine) {
    if (!ensureTexture(machine)) return;
    const VbeModeInfo &mode = machine.vbe().currentMode();
    const std::vector<uint8_t> &fb = machine.vbe().framebuffer();
    rgb_buffer_.assign(static_cast<size_t>(mode.width) * mode.height * 3u, 0);

    for (uint16_t y = 0; y < mode.height; y++) {
      const uint8_t *src = fb.data() + static_cast<size_t>(y) * mode.pitch;
      uint8_t *dst = rgb_buffer_.data() + static_cast<size_t>(y) * mode.width * 3u;
      for (uint16_t x = 0; x < mode.width; x++) {
        const uint8_t *px = src + static_cast<size_t>(x) * mode.bytes_per_pixel;
        uint8_t *out = dst + static_cast<size_t>(x) * 3u;
        if (mode.bytes_per_pixel == 1) {
          out[0] = out[1] = out[2] = px[0];
        } else if (mode.bytes_per_pixel == 2) {
          uint16_t v = static_cast<uint16_t>(px[0] | (px[1] << 8));
          out[0] = static_cast<uint8_t>(((v >> 10) & 0x1F) * 255 / 31);
          out[1] = static_cast<uint8_t>(((v >> 5) & 0x1F) * 255 / 31);
          out[2] = static_cast<uint8_t>((v & 0x1F) * 255 / 31);
        } else {
          out[0] = px[2];
          out[1] = px[1];
          out[2] = px[0];
        }
      }
    }

    SDL_UpdateTexture(texture_, nullptr, rgb_buffer_.data(), static_cast<int>(mode.width) * 3);
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 255);
    SDL_RenderClear(renderer_);
    SDL_RenderTexture(renderer_, texture_, nullptr, nullptr);
  }

  void renderTerminal() {
    SDL_SetRenderDrawColor(renderer_, 17, 20, 24, 255);
    SDL_RenderClear(renderer_);

    int y = 46;
    int line_height = font_ != nullptr ? TTF_GetFontHeight(font_) + 3 : 17;
    size_t max_lines = logical_height_ > 70 ? static_cast<size_t>((logical_height_ - 70) / line_height) : 1;
    size_t start = terminal_lines_.size() > max_lines ? terminal_lines_.size() - max_lines : 0;

    for (size_t i = start; i < terminal_lines_.size(); i++) {
      drawText(18, y, terminal_lines_[i], rgba(224, 231, 238), font_);
      y += line_height;
    }
    if (!current_line_.empty() && y < logical_height_ - line_height) {
      drawText(18, y, current_line_, rgba(224, 231, 238), font_);
    }
  }

  void renderChrome(Machine &machine) {
    SDL_SetRenderDrawBlendMode(renderer_, SDL_BLENDMODE_BLEND);
    SDL_FRect top{0.0f, 0.0f, static_cast<float>(logical_width_), 32.0f};
    SDL_SetRenderDrawColor(renderer_, 24, 29, 36, 230);
    SDL_RenderFillRect(renderer_, &top);
    SDL_SetRenderDrawColor(renderer_, 76, 86, 99, 255);
    SDL_RenderLine(renderer_, 0.0f, 31.0f, static_cast<float>(logical_width_), 31.0f);

    const char *mode = machine.vbe().graphicsMode() ? "LCOM Display" : "LCOM Terminal";
    drawText(12, 7, mode, rgba(246, 248, 250), title_font_);

    drawText(logical_width_ - 98, 7, debug_visible_ ? "F3 debug on" : "F3 debug",
             rgba(192, 205, 218), title_font_);

    if (debug_visible_) renderDebugPanel(machine);
  }

  void renderDebugPanel(Machine &machine) {
    int w = std::min(420, logical_width_ - 32);
    SDL_FRect panel{static_cast<float>(logical_width_ - w - 14), 44.0f,
                    static_cast<float>(w), static_cast<float>(logical_height_ - 58)};
    SDL_SetRenderDrawColor(renderer_, 19, 23, 29, 238);
    SDL_RenderFillRect(renderer_, &panel);
    SDL_SetRenderDrawColor(renderer_, 92, 108, 123, 255);
    SDL_RenderRect(renderer_, &panel);

    int x = static_cast<int>(panel.x) + 14;
    int y = static_cast<int>(panel.y) + 12;
    int lh = font_ != nullptr ? TTF_GetFontHeight(font_) + 4 : 18;

    auto line = [&](const std::string &text, SDL_Color color = rgba(228, 233, 239)) {
      drawText(x, y, text, color, font_);
      y += lh;
    };

    char buf[256];
    line("LCOMBus Debug", rgba(113, 221, 237));
    std::snprintf(buf, sizeof(buf), "tick: %llu", static_cast<unsigned long long>(machine.tick()));
    line(buf);
    std::snprintf(buf, sizeof(buf), "pending IRQ mask: 0x%08x", machine.pendingIrqs());
    line(buf);

    y += 4;
    std::snprintf(buf, sizeof(buf), "PIT ch0 divisor: %u", machine.pit().divisor(0));
    line(buf);
    std::snprintf(buf, sizeof(buf), "PIT ch0 frequency: %u Hz", machine.pit().channelFrequency(0));
    line(buf);

    y += 4;
    std::snprintf(buf, sizeof(buf), "i8042 status: 0x%02x", machine.i8042().status());
    line(buf);
    std::snprintf(buf, sizeof(buf), "i8042 command byte: 0x%02x", machine.i8042().commandByte());
    line(buf);
    line(machine.i8042().mouseReporting() ? "mouse reporting: enabled" : "mouse reporting: disabled");
    line(mouse_captured_ ? "mouse capture: relative" : "mouse capture: released");
    std::snprintf(buf, sizeof(buf), "mouse sample cap: %u Hz", 60u);
    line(buf);

    y += 4;
    const VbeModeInfo &vbe = machine.vbe().currentMode();
    std::snprintf(buf, sizeof(buf), "VBE mode: 0x%03x", vbe.mode);
    line(buf);
    std::snprintf(buf, sizeof(buf), "framebuffer: %ux%u %u bpp", vbe.width, vbe.height, vbe.bpp);
    line(buf);
    std::snprintf(buf, sizeof(buf), "pitch: %u bytes", vbe.pitch);
    line(buf);
    std::snprintf(buf, sizeof(buf), "phys: 0x%llx", static_cast<unsigned long long>(vbe.framebuffer_phys));
    line(buf);

    y += 4;
    std::snprintf(buf, sizeof(buf), "host frame: %.1f fps  %.2f ms", current_fps_, current_frame_ms_);
    line(buf, rgba(153, 221, 255));
    drawFrameChart(x, y, w - 28, 72);
    y += 84;

    y += 4;
    line("Keys: F3 debug, F11 fullscreen", rgba(172, 184, 196));
  }

  void recordFrameTiming() {
    uint64_t now = SDL_GetTicksNS();
    if (last_present_ns_ != 0) {
      current_frame_ms_ = static_cast<float>((now - last_present_ns_) / 1000000.0);
      current_fps_ = current_frame_ms_ > 0.0f ? 1000.0f / current_frame_ms_ : 0.0f;
      frame_ms_samples_.push_back(current_frame_ms_);
      if (frame_ms_samples_.size() > 180) {
        frame_ms_samples_.erase(frame_ms_samples_.begin(),
                                frame_ms_samples_.begin() + (frame_ms_samples_.size() - 180));
      }
    }
    last_present_ns_ = now;
  }

  void drawFrameChart(int x, int y, int w, int h) {
    if (renderer_ == nullptr) return;
    SDL_FRect chart{static_cast<float>(x), static_cast<float>(y),
                    static_cast<float>(w), static_cast<float>(h)};
    SDL_SetRenderDrawColor(renderer_, 10, 15, 21, 235);
    SDL_RenderFillRect(renderer_, &chart);
    SDL_SetRenderDrawColor(renderer_, 55, 69, 84, 255);
    SDL_RenderRect(renderer_, &chart);

    float target_y = static_cast<float>(y + h) - (16.67f / 50.0f) * static_cast<float>(h);
    SDL_SetRenderDrawColor(renderer_, 70, 92, 112, 255);
    SDL_RenderLine(renderer_, static_cast<float>(x), target_y,
                   static_cast<float>(x + w), target_y);

    if (frame_ms_samples_.size() < 2) return;
    SDL_SetRenderDrawColor(renderer_, 113, 221, 237, 255);
    size_t first = frame_ms_samples_.size() > static_cast<size_t>(w)
                       ? frame_ms_samples_.size() - static_cast<size_t>(w)
                       : 0;
    float prev_x = static_cast<float>(x);
    float prev_y = chartY(y, h, frame_ms_samples_[first]);
    for (size_t i = first + 1; i < frame_ms_samples_.size(); i++) {
      float px = static_cast<float>(x + static_cast<int>(i - first));
      float py = chartY(y, h, frame_ms_samples_[i]);
      SDL_RenderLine(renderer_, prev_x, prev_y, px, py);
      prev_x = px;
      prev_y = py;
    }
  }

  static float chartY(int y, int h, float frame_ms) {
    float clamped = std::max(0.0f, std::min(50.0f, frame_ms));
    return static_cast<float>(y + h) - (clamped / 50.0f) * static_cast<float>(h);
  }

  void drawText(int x, int y, const std::string &text, SDL_Color color, TTF_Font *font) {
    if (renderer_ == nullptr || font == nullptr || text.empty()) return;
    SDL_Surface *surface = TTF_RenderText_Blended(font, text.c_str(), text.size(), color);
    if (surface == nullptr) return;
    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer_, surface);
    if (texture != nullptr) {
      SDL_FRect dst{static_cast<float>(x), static_cast<float>(y),
                    static_cast<float>(surface->w), static_cast<float>(surface->h)};
      SDL_RenderTexture(renderer_, texture, nullptr, &dst);
      SDL_DestroyTexture(texture);
    }
    SDL_DestroySurface(surface);
  }

  bool isChromePoint(int x, int y) const {
    return y < 32 || (debug_visible_ && x > logical_width_ - 440 && y > 40);
  }

  void logicalPoint(float window_x, float window_y, int &x, int &y) const {
    float rx = window_x;
    float ry = window_y;
    if (renderer_ != nullptr) SDL_RenderCoordinatesFromWindow(renderer_, window_x, window_y, &rx, &ry);
    x = static_cast<int>(rx);
    y = static_cast<int>(ry);
  }

  static uint8_t buttonsFromState(SDL_MouseButtonFlags state) {
    uint8_t buttons = 0;
    if (state & SDL_BUTTON_LMASK) buttons |= 0x01;
    if (state & SDL_BUTTON_RMASK) buttons |= 0x02;
    if (state & SDL_BUTTON_MMASK) buttons |= 0x04;
    return buttons;
  }

  void updateHostButton(uint8_t button, bool down) {
    uint8_t bit = 0;
    if (button == SDL_BUTTON_LEFT) bit = 0x01;
    if (button == SDL_BUTTON_RIGHT) bit = 0x02;
    if (button == SDL_BUTTON_MIDDLE) bit = 0x04;
    if (bit == 0) return;
    if (down) {
      host_mouse_buttons_ |= bit;
    } else {
      host_mouse_buttons_ &= static_cast<uint8_t>(~bit);
    }
  }

  void sampleMouse(Machine &machine, bool force) {
    if (window_ == nullptr || renderer_ == nullptr || !mouse_captured_) return;
    uint64_t now = SDL_GetTicksNS();
    if (!force && now - last_mouse_sample_ns_ < kMouseSampleIntervalNs) {
      return;
    }

    float rx = 0.0f;
    float ry = 0.0f;
    SDL_MouseButtonFlags state = SDL_GetRelativeMouseState(&rx, &ry);
    host_mouse_buttons_ = buttonsFromState(state);

    relative_mouse_x_ += rx;
    relative_mouse_y_ += ry;
    int screen_dx = static_cast<int>(std::round(relative_mouse_x_));
    int screen_dy = static_cast<int>(std::round(relative_mouse_y_));
    screen_dx = std::max(-kMouseStep, std::min(kMouseStep, screen_dx));
    screen_dy = std::max(-kMouseStep, std::min(kMouseStep, screen_dy));
    if (screen_dx != 0 || screen_dy != 0) {
      relative_mouse_x_ -= static_cast<double>(screen_dx);
      relative_mouse_y_ -= static_cast<double>(screen_dy);
      machine.injectMouse(screen_dx, -screen_dy, host_mouse_buttons_);
      guest_mouse_x_ = std::max(0, std::min(logical_width_ - 1, guest_mouse_x_ + screen_dx));
      guest_mouse_y_ = std::max(0, std::min(logical_height_ - 1, guest_mouse_y_ + screen_dy));
    }
    last_mouse_sample_ns_ = now;
  }

  void updateMouseCapture(Machine &machine) {
    if (window_ == nullptr) return;
    SDL_WindowFlags flags = SDL_GetWindowFlags(window_);
    bool focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    bool should_capture = focused && machine.i8042().mouseReporting() && !debug_visible_;
    setMouseCaptured(should_capture);
  }

  void setMouseCaptured(bool captured) {
    if (window_ == nullptr || mouse_captured_ == captured) return;
    if (captured) {
      SDL_SetWindowMouseGrab(window_, true);
      SDL_SetWindowRelativeMouseMode(window_, true);
      SDL_HideCursor();
      relative_mouse_x_ = 0.0;
      relative_mouse_y_ = 0.0;
      float discard_x = 0.0f;
      float discard_y = 0.0f;
      SDL_GetRelativeMouseState(&discard_x, &discard_y);
      last_mouse_sample_ns_ = SDL_GetTicksNS();
    } else {
      SDL_SetWindowRelativeMouseMode(window_, false);
      SDL_SetWindowMouseGrab(window_, false);
      SDL_ShowCursor();
      relative_mouse_x_ = 0.0;
      relative_mouse_y_ = 0.0;
      float discard_x = 0.0f;
      float discard_y = 0.0f;
      SDL_GetRelativeMouseState(&discard_x, &discard_y);
    }
    mouse_captured_ = captured;
  }

  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *texture_ = nullptr;
  TTF_Font *font_ = nullptr;
  TTF_Font *title_font_ = nullptr;

  SdlBackendOptions options_{};
  bool debug_visible_ = false;
  bool fullscreen_ = false;
  bool should_quit_ = false;
  bool mouse_captured_ = false;
  uint8_t host_mouse_buttons_ = 0;
  int guest_mouse_x_ = 400;
  int guest_mouse_y_ = 300;
  double relative_mouse_x_ = 0.0;
  double relative_mouse_y_ = 0.0;
  uint64_t last_mouse_sample_ns_ = 0;
  uint64_t last_present_ns_ = 0;
  float current_frame_ms_ = 0.0f;
  float current_fps_ = 0.0f;
  int logical_width_ = 800;
  int logical_height_ = 600;
  int texture_width_ = 0;
  int texture_height_ = 0;

  std::vector<std::string> terminal_lines_;
  std::string current_line_;
  std::vector<uint8_t> rgb_buffer_;
  std::vector<float> frame_ms_samples_;
};

std::unique_ptr<DisplayBackend> createSdlBackend(const SdlBackendOptions &options) {
  return std::unique_ptr<DisplayBackend>(new SdlBackend(options));
}

} // namespace lcom

#endif
