#if defined(LCOM_WITH_SDL)

#include "SdlBackend.hpp"

#include "../core/MousePacketScheduler.hpp"

#include <SDL3/SDL.h>
#include <SDL3_ttf/SDL_ttf.h>
#include <lcom/i8042.h>

#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
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
constexpr int kMaxSdlEventsPerPump = 256;
constexpr uint64_t kStartupHintNs = 5000000000ull;

int g_sdl_backend_users = 0;

bool acquireSdl() {
  if (g_sdl_backend_users == 0) {
    SDL_SetHint(SDL_HINT_MOUSE_EMULATE_WARP_WITH_RELATIVE, "0");
    if (!SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS | SDL_INIT_AUDIO)) {
      std::fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
      return false;
    }
    if (!TTF_Init()) {
      std::fprintf(stderr, "TTF_Init failed: %s\n", SDL_GetError());
      SDL_Quit();
      return false;
    }
    SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, false);
  }
  g_sdl_backend_users++;
  return true;
}

void releaseSdl() {
  if (g_sdl_backend_users <= 0) return;
  g_sdl_backend_users--;
  if (g_sdl_backend_users == 0) {
    SDL_SetEventEnabled(SDL_EVENT_MOUSE_MOTION, true);
    TTF_Quit();
    SDL_Quit();
  }
}

} // namespace

class SdlBackend final : public DisplayBackend, public InputObserver {
public:
  explicit SdlBackend(SdlBackendOptions options) : options_(std::move(options)) {}

  bool start(Machine &machine) override {
    if (!acquireSdl()) return false;
    sdl_acquired_ = true;

    const VbeModeInfo &mode = machine.vbe().currentMode();
    logical_width_ = mode.width;
    logical_height_ = mode.height;

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
    window_id_ = SDL_GetWindowID(window_);

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
    observed_machine_ = &machine;
    machine.setInputObserver(this);
    startup_hint_until_ns_ = SDL_GetTicksNS() + kStartupHintNs;
    return ensureTexture(machine);
  }

  void onKeyScancode(uint8_t byte) override {
    char buf[16];
    std::snprintf(buf, sizeof(buf), "0x%02x", byte);
    pushHistory(scancode_history_, buf, 16);
  }

  void onMousePacket(int dx, int dy, uint8_t buttons) override {
    int packet_dx = std::max(-255, std::min(255, dx));
    int packet_dy = std::max(-255, std::min(255, dy));
    uint8_t b0 = MOUSE_SYNC_BIT;
    if (buttons & 0x01u) b0 |= BIT(0);
    if (buttons & 0x02u) b0 |= BIT(1);
    if (buttons & 0x04u) b0 |= BIT(2);
    if (packet_dx < 0) b0 |= BIT(4);
    if (packet_dy < 0) b0 |= BIT(5);

    char buf[96];
    std::snprintf(buf, sizeof(buf), "%02x %02x %02x  dx=%d dy=%d  %c%c%c",
                  b0, static_cast<uint8_t>(packet_dx & 0xFF),
                  static_cast<uint8_t>(packet_dy & 0xFF),
                  packet_dx, packet_dy,
                  (buttons & 0x01u) ? 'L' : '-',
                  (buttons & 0x04u) ? 'M' : '-',
                  (buttons & 0x02u) ? 'R' : '-');
    pushHistory(mouse_packet_history_, buf, 12);
  }

  void pump(Machine &machine) override {
    SDL_Event ev;
    std::vector<SDL_Event> foreign_events;

    if (isHostMouseReleaseHeld()) {
      mouse_capture_suppressed_ = true;
      setMouseCaptured(false, true);
    }
    updateMouseCapture(machine);
    if (options_.guest_input) sampleRelativeMouse(machine, false);
    int polled = 0;
    while (polled < kMaxSdlEventsPerPump && SDL_PollEvent(&ev)) {
      polled++;
      if (!eventBelongsToThisWindow(ev)) {
        foreign_events.push_back(ev);
        continue;
      }
      switch (ev.type) {
      case SDL_EVENT_QUIT:
      case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
        should_quit_ = true;
        machine.injectKey("ESC", true);
        machine.injectKey("ESC", false);
        break;
      case SDL_EVENT_WINDOW_FOCUS_LOST:
      case SDL_EVENT_WINDOW_MOUSE_LEAVE:
        mouse_capture_suppressed_ = true;
        setMouseCaptured(false, true);
        break;
      case SDL_EVENT_WINDOW_FOCUS_GAINED:
      case SDL_EVENT_WINDOW_MOUSE_ENTER:
        updateMouseCapture(machine);
        break;
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP:
        if (options_.guest_input) flushMousePacket(machine, true);
        handleKey(machine, ev.key);
        break;
      case SDL_EVENT_MOUSE_MOTION:
        break;
      case SDL_EVENT_MOUSE_BUTTON_DOWN:
      case SDL_EVENT_MOUSE_BUTTON_UP: {
        int lx = 0;
        int ly = 0;
        logicalPoint(ev.button.x, ev.button.y, lx, ly);
        if (debug_visible_ && ev.button.down && handleDebugClick(machine, lx, ly)) {
          break;
        }
        if (!options_.guest_input) break;
        if (mouse_capture_suppressed_ && ev.button.down && machine.i8042().mouseReporting() &&
            !debug_visible_ && !isChromePoint(lx, ly)) {
          mouse_capture_suppressed_ = false;
          setMouseCaptured(true);
          break;
        }
        if (canInjectGuestMouse(machine) ||
            (!isChromePoint(lx, ly) && machine.i8042().mouseReporting() && !debug_visible_)) {
          updateMouseCapture(machine);
          updateHostButton(ev.button.button, ev.button.down);
          mouse_scheduler_.setButtons(host_mouse_buttons_);
          flushMousePacket(machine, true);
        }
        break;
      }
      default:
        break;
      }
    }
    for (SDL_Event &foreign : foreign_events) {
      SDL_PushEvent(&foreign);
    }
    if (isHostMouseReleaseHeld()) {
      mouse_capture_suppressed_ = true;
      setMouseCaptured(false, true);
    }
    updateMouseCapture(machine);
    if (options_.guest_input) sampleRelativeMouse(machine, false);
    if (options_.guest_input) flushMousePacket(machine, false);
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
    if (observed_machine_ != nullptr) observed_machine_->setInputObserver(nullptr);
    setMouseCaptured(false);
    if (texture_ != nullptr) SDL_DestroyTexture(texture_);
    if (font_ != nullptr) TTF_CloseFont(font_);
    if (title_font_ != nullptr) TTF_CloseFont(title_font_);
    if (renderer_ != nullptr) SDL_DestroyRenderer(renderer_);
    if (window_ != nullptr) SDL_DestroyWindow(window_);
    if (sdl_acquired_) releaseSdl();
  }

private:
  bool ensureTexture(Machine &machine) {
    const VbeModeInfo &mode = machine.vbe().currentMode();
    SDL_PixelFormat desired_format =
        mode.bytes_per_pixel == 3 ? SDL_PIXELFORMAT_BGR24 : SDL_PIXELFORMAT_RGB24;
    if (texture_ != nullptr && texture_width_ == mode.width && texture_height_ == mode.height &&
        texture_format_ == desired_format) {
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
    texture_format_ = desired_format;
    texture_ = SDL_CreateTexture(renderer_, texture_format_, SDL_TEXTUREACCESS_STREAMING,
                                 texture_width_, texture_height_);
    if (texture_ == nullptr) {
      std::fprintf(stderr, "SDL_CreateTexture failed: %s\n", SDL_GetError());
      return false;
    }
    return true;
  }

  void handleKey(Machine &machine, const SDL_KeyboardEvent &key) {
    bool down = key.down;
    if (save_modal_visible_) {
      if (down && !key.repeat) handleSaveModalKey(key);
      return;
    }
    if (isHostMouseReleaseKey(key.scancode)) {
      mouse_capture_suppressed_ = true;
      setMouseCaptured(false, true);
      return;
    }
    if (down && key.scancode == SDL_SCANCODE_F9) {
      beginRecording(machine);
      return;
    }
    if (down && key.scancode == SDL_SCANCODE_F8) {
      toggleRecordingPause(machine);
      return;
    }
    if (down && key.scancode == SDL_SCANCODE_F10) {
      finishRecording(machine);
      return;
    }
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
    if (!options_.guest_input) return;
    if (key.repeat) return;

    std::string guest_key = guestKeyName(key);
    if (!guest_key.empty()) {
      recordKey(machine, guest_key, down);
      machine.injectKey(guest_key, down);
    }
  }

  void renderFramebuffer(Machine &machine) {
    if (!ensureTexture(machine)) return;
    const VbeModeInfo &mode = machine.vbe().currentMode();
    const std::vector<uint8_t> &fb = machine.vbe().framebuffer();
    if (mode.bytes_per_pixel == 3) {
      SDL_UpdateTexture(texture_, nullptr, fb.data(), static_cast<int>(mode.pitch));
    } else {
      size_t needed = static_cast<size_t>(mode.width) * mode.height * 3u;
      if (rgb_buffer_.size() != needed) rgb_buffer_.assign(needed, 0);

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
    }
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
    if (debug_visible_) {
      renderInputPanel();
      renderDebugPanel(machine);
    } else {
      renderStartupHint();
    }
    renderRecordingStatus();
    if (save_modal_visible_) renderSaveModal();
  }

  void renderStartupHint() {
    uint64_t now = SDL_GetTicksNS();
    if (startup_hint_until_ns_ == 0 || now >= startup_hint_until_ns_) return;

    float remaining = static_cast<float>(startup_hint_until_ns_ - now) /
                      static_cast<float>(kStartupHintNs);
    uint8_t alpha = remaining < 0.35f
                        ? static_cast<uint8_t>(220.0f * (remaining / 0.35f))
                        : 220u;
    const char *label = "F3 to debug";
    int w = 124;
    int h = 30;
    int x = logical_width_ - w - 14;
    int y = 14;
    SDL_FRect box{static_cast<float>(x), static_cast<float>(y),
                  static_cast<float>(w), static_cast<float>(h)};
    SDL_SetRenderDrawColor(renderer_, 19, 23, 29, alpha);
    SDL_RenderFillRect(renderer_, &box);
    SDL_SetRenderDrawColor(renderer_, 113, 221, 237, alpha);
    SDL_RenderRect(renderer_, &box);
    drawText(x + 12, y + 7, label, rgba(228, 233, 239, alpha), title_font_);
  }

  void renderInputPanel() {
    int w = std::min(310, logical_width_ / 2 - 28);
    SDL_FRect panel{14.0f, 14.0f, static_cast<float>(w),
                    static_cast<float>(logical_height_ - 28)};
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

    line("Input Stream", rgba(113, 221, 237));
    y += 4;
    line("Latest scancodes", rgba(153, 221, 255));
    if (scancode_history_.empty()) {
      line("(none)", rgba(134, 148, 162));
    } else {
      for (auto it = scancode_history_.rbegin(); it != scancode_history_.rend(); ++it) {
        line(*it);
      }
    }

    y += 8;
    line("Latest mouse packets", rgba(153, 221, 255));
    if (mouse_packet_history_.empty()) {
      line("(none)", rgba(134, 148, 162));
    } else {
      for (auto it = mouse_packet_history_.rbegin(); it != mouse_packet_history_.rend(); ++it) {
        if (y > logical_height_ - lh - 16) break;
        line(*it);
      }
    }
  }

  void renderDebugPanel(Machine &machine) {
    int w = std::min(420, logical_width_ - 32);
    SDL_FRect panel{static_cast<float>(logical_width_ - w - 14), 14.0f,
                    static_cast<float>(w), static_cast<float>(logical_height_ - 28)};
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
    line(mouse_captured_ ? "mouse capture: grabbed" : "mouse capture: released");
    std::snprintf(buf, sizeof(buf), "mouse sample cap: %u Hz", 60u);
    line(buf);
    std::snprintf(buf, sizeof(buf), "mouse backlog: dx=%d dy=%d",
                  mouse_scheduler_.pendingDx(), mouse_scheduler_.pendingScreenDy());
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
    drawFrameChart(x, y, w - 28, 124);
    y += 136;

    y += 8;
    line("Record", rgba(153, 221, 255));
    int button_y = y;
    int button_w = recording_active_ ? 118 : 144;
    drawDebugButton(record_button_, x, button_y, button_w,
                    recording_active_ ? "Recording" : "F9 Record",
                    recording_active_ ? rgba(252, 211, 77) : rgba(228, 233, 239));
    if (recording_active_) {
      drawDebugButton(pause_button_, x + 126, button_y, 118,
                      recording_paused_ ? "F8 In" : "F8 Out",
                      recording_paused_ ? rgba(113, 221, 237) : rgba(228, 233, 239));
      drawDebugButton(stop_button_, x + 252, button_y, 118, "F10 Save",
                      rgba(228, 233, 239));
    } else {
      pause_button_ = SDL_FRect{0.0f, 0.0f, 0.0f, 0.0f};
      stop_button_ = SDL_FRect{0.0f, 0.0f, 0.0f, 0.0f};
    }
  }

  void drawDebugButton(SDL_FRect &rect, int x, int y, int w, const char *label, SDL_Color color) {
    int h = 28;
    rect = SDL_FRect{static_cast<float>(x), static_cast<float>(y),
                     static_cast<float>(w), static_cast<float>(h)};
    SDL_SetRenderDrawColor(renderer_, 35, 44, 55, 245);
    SDL_RenderFillRect(renderer_, &rect);
    SDL_SetRenderDrawColor(renderer_, 92, 108, 123, 255);
    SDL_RenderRect(renderer_, &rect);
    drawText(static_cast<int>(rect.x) + 10, static_cast<int>(rect.y) + 6, label, color, font_);
  }

  void renderRecordingStatus() {
    if (!recording_active_ && !recording_paused_ && record_status_until_ns_ <= SDL_GetTicksNS()) return;
    const char *label = recording_paused_ ? "Paused" : (recording_active_ ? "Recording" : record_status_.c_str());
    if (label == nullptr || label[0] == '\0') return;
    int w = recording_paused_ ? 96 : 150;
    int h = 30;
    int x = logical_width_ - w - 14;
    int y = logical_height_ - h - 14;
    SDL_FRect box{static_cast<float>(x), static_cast<float>(y),
                  static_cast<float>(w), static_cast<float>(h)};
    SDL_SetRenderDrawColor(renderer_, recording_paused_ ? 92 : 80,
                           recording_paused_ ? 55 : 24,
                           recording_paused_ ? 20 : 34, 220);
    SDL_RenderFillRect(renderer_, &box);
    SDL_SetRenderDrawColor(renderer_, recording_paused_ ? 252 : 248,
                           recording_paused_ ? 211 : 113,
                           recording_paused_ ? 77 : 113, 255);
    SDL_RenderRect(renderer_, &box);
    drawText(x + 12, y + 7, label, rgba(248, 250, 252), title_font_);
  }

  void renderSaveModal() {
    int w = std::min(560, logical_width_ - 64);
    int h = 148;
    int x = (logical_width_ - w) / 2;
    int y = (logical_height_ - h) / 2;
    SDL_FRect shade{0.0f, 0.0f, static_cast<float>(logical_width_), static_cast<float>(logical_height_)};
    SDL_SetRenderDrawColor(renderer_, 0, 0, 0, 145);
    SDL_RenderFillRect(renderer_, &shade);
    SDL_FRect box{static_cast<float>(x), static_cast<float>(y),
                  static_cast<float>(w), static_cast<float>(h)};
    SDL_SetRenderDrawColor(renderer_, 20, 26, 34, 250);
    SDL_RenderFillRect(renderer_, &box);
    SDL_SetRenderDrawColor(renderer_, 113, 221, 237, 255);
    SDL_RenderRect(renderer_, &box);
    drawText(x + 18, y + 18, "Save recording script", rgba(228, 233, 239), title_font_);
    SDL_FRect input{static_cast<float>(x + 18), static_cast<float>(y + 58),
                    static_cast<float>(w - 36), 34.0f};
    SDL_SetRenderDrawColor(renderer_, 9, 14, 21, 255);
    SDL_RenderFillRect(renderer_, &input);
    SDL_SetRenderDrawColor(renderer_, 92, 108, 123, 255);
    SDL_RenderRect(renderer_, &input);
    drawText(x + 28, y + 66, save_path_input_ + "_", rgba(246, 248, 250), font_);
    drawText(x + 18, y + 108, "Enter saves, Esc cancels", rgba(172, 184, 196), font_);
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

    auto guide = [&](float frame_ms, uint8_t r, uint8_t g, uint8_t b) {
      float gy = chartY(y, h, frame_ms);
      SDL_SetRenderDrawColor(renderer_, r, g, b, 255);
      SDL_RenderLine(renderer_, static_cast<float>(x), gy,
                     static_cast<float>(x + w), gy);
    };
    guide(16.67f, 70, 92, 112);
    guide(33.33f, 58, 70, 84);

    drawText(x + 6, y + 5, "16.7", rgba(126, 142, 158), font_);
    drawText(x + 6, y + h - 22, "50ms", rgba(126, 142, 158), font_);

    if (frame_ms_samples_.size() < 2) return;
    SDL_SetRenderDrawColor(renderer_, 113, 221, 237, 255);
    size_t first = frame_ms_samples_.size() > 240 ? frame_ms_samples_.size() - 240 : 0;
    size_t count = frame_ms_samples_.size() - first;
    auto sampleX = [&](size_t index) {
      float t = count <= 1 ? 0.0f : static_cast<float>(index) / static_cast<float>(count - 1);
      return static_cast<float>(x) + t * static_cast<float>(w - 1);
    };
    float prev_x = sampleX(0);
    float prev_y = chartY(y, h, frame_ms_samples_[first]);
    for (size_t i = first + 1; i < frame_ms_samples_.size(); i++) {
      float px = sampleX(i - first);
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

  static void pushHistory(std::vector<std::string> &history,
                          const std::string &line,
                          size_t limit) {
    history.push_back(line);
    if (history.size() > limit) {
      history.erase(history.begin(), history.begin() + (history.size() - limit));
    }
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
    if (!debug_visible_) return false;
    int left_w = std::min(310, logical_width_ / 2 - 28);
    int right_w = std::min(420, logical_width_ - 32);
    bool in_left = x >= 14 && x < 14 + left_w && y >= 44 && y < logical_height_ - 14;
    bool in_right = x >= logical_width_ - right_w - 14 && y >= 44 && y < logical_height_ - 14;
    return in_left || in_right;
  }

  static bool isHostMouseReleaseKey(SDL_Scancode scancode) {
    return scancode == SDL_SCANCODE_LGUI ||
           scancode == SDL_SCANCODE_RGUI ||
           scancode == SDL_SCANCODE_RCTRL;
  }

  static bool isHostMouseReleaseHeld() {
    if ((SDL_GetModState() & SDL_KMOD_GUI) != 0) return true;
    int key_count = 0;
    const bool *keys = SDL_GetKeyboardState(&key_count);
    return keys != nullptr && SDL_SCANCODE_RCTRL < key_count && keys[SDL_SCANCODE_RCTRL];
  }

  bool eventBelongsToThisWindow(const SDL_Event &ev) const {
    if (window_id_ == 0) return true;
    uint32_t event_window = 0;
    switch (ev.type) {
    case SDL_EVENT_WINDOW_CLOSE_REQUESTED:
    case SDL_EVENT_WINDOW_FOCUS_LOST:
    case SDL_EVENT_WINDOW_FOCUS_GAINED:
    case SDL_EVENT_WINDOW_MOUSE_LEAVE:
    case SDL_EVENT_WINDOW_MOUSE_ENTER:
      event_window = ev.window.windowID;
      break;
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP:
      event_window = ev.key.windowID;
      break;
    case SDL_EVENT_MOUSE_MOTION:
      event_window = ev.motion.windowID;
      break;
    case SDL_EVENT_MOUSE_BUTTON_DOWN:
    case SDL_EVENT_MOUSE_BUTTON_UP:
      event_window = ev.button.windowID;
      break;
    case SDL_EVENT_MOUSE_WHEEL:
      event_window = ev.wheel.windowID;
      break;
    default:
      return true;
    }
    return event_window == 0 || event_window == window_id_;
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

  bool canInjectGuestMouse(Machine &machine) const {
    return options_.guest_input && mouse_captured_ && machine.i8042().mouseReporting() && !debug_visible_;
  }

  void queueMouseMotion(int dx, int screen_dy) {
    mouse_scheduler_.addMotion(dx, screen_dy);
  }

  void sampleRelativeMouse(Machine &machine, bool force) {
    if (window_ == nullptr || !mouse_captured_) return;
    float rel_x = 0.0f;
    float rel_y = 0.0f;
    SDL_MouseButtonFlags state = SDL_GetRelativeMouseState(&rel_x, &rel_y);
    if (drop_next_relative_sample_) {
      drop_next_relative_sample_ = false;
      rel_x = 0.0f;
      rel_y = 0.0f;
    }
    uint8_t buttons = buttonsFromState(state);
    if (buttons != host_mouse_buttons_) {
      host_mouse_buttons_ = buttons;
      mouse_scheduler_.setButtons(host_mouse_buttons_);
      force = true;
    }
    if (canInjectGuestMouse(machine)) {
      queueMouseMotion(static_cast<int>(rel_x), static_cast<int>(rel_y));
      flushMousePacket(machine, force);
    }
  }

  void flushMousePacket(Machine &machine, bool force) {
    if (window_ == nullptr || renderer_ == nullptr || !mouse_captured_) return;
    uint64_t now = SDL_GetTicksNS();
    if (!force && now - last_mouse_sample_ns_ < kMouseSampleIntervalNs) {
      return;
    }

    auto packet = mouse_scheduler_.nextPacket(force);
    if (!packet) return;
    machine.injectMouse(packet->dx, packet->dy, packet->buttons);
    recordMouse(machine, packet->dx, packet->dy, packet->buttons);
    last_mouse_sample_ns_ = now;
  }

  void updateMouseCapture(Machine &machine) {
    if (window_ == nullptr) return;
    SDL_WindowFlags flags = SDL_GetWindowFlags(window_);
    bool focused = (flags & SDL_WINDOW_INPUT_FOCUS) != 0;
    bool should_capture = options_.guest_input && focused && machine.i8042().mouseReporting() && !debug_visible_ &&
                          !mouse_capture_suppressed_;
    setMouseCaptured(should_capture);
  }

  void setMouseCaptured(bool captured, bool force = false) {
    if (window_ == nullptr || (!force && mouse_captured_ == captured)) return;
    if (captured) {
      SDL_CaptureMouse(true);
      SDL_SetWindowMouseGrab(window_, true);
      SDL_SetWindowRelativeMouseMode(window_, true);
      SDL_GetRelativeMouseState(nullptr, nullptr);
      mouse_scheduler_.reset();
      last_mouse_sample_ns_ = SDL_GetTicksNS();
      drop_next_relative_sample_ = true;
    } else {
      SDL_SetWindowRelativeMouseMode(window_, false);
      SDL_SetWindowMouseGrab(window_, false);
      SDL_SetWindowKeyboardGrab(window_, false);
      SDL_SetWindowMouseRect(window_, nullptr);
      SDL_CaptureMouse(false);
      SDL_ShowCursor();
      SDL_GetRelativeMouseState(nullptr, nullptr);
      mouse_scheduler_.reset();
      host_mouse_buttons_ = 0;
    }
    mouse_captured_ = captured;
  }

  bool handleDebugClick(Machine &machine, int x, int y) {
    auto hit = [&](const SDL_FRect &r) {
      return x >= static_cast<int>(r.x) && x < static_cast<int>(r.x + r.w) &&
             y >= static_cast<int>(r.y) && y < static_cast<int>(r.y + r.h);
    };
    if (hit(record_button_)) {
      beginRecording(machine);
      return true;
    }
    if (recording_active_ && hit(pause_button_)) {
      toggleRecordingPause(machine);
      return true;
    }
    if (recording_active_ && hit(stop_button_)) {
      finishRecording(machine);
      return true;
    }
    return false;
  }

  static std::string guestKeyName(const SDL_KeyboardEvent &key) {
    switch (key.scancode) {
    case SDL_SCANCODE_ESCAPE: return "ESC";
    case SDL_SCANCODE_1: return "1";
    case SDL_SCANCODE_2: return "2";
    case SDL_SCANCODE_3: return "3";
    case SDL_SCANCODE_4: return "4";
    case SDL_SCANCODE_5: return "5";
    case SDL_SCANCODE_6: return "6";
    case SDL_SCANCODE_7: return "7";
    case SDL_SCANCODE_8: return "8";
    case SDL_SCANCODE_9: return "9";
    case SDL_SCANCODE_0: return "0";
    case SDL_SCANCODE_MINUS: return "MINUS";
    case SDL_SCANCODE_EQUALS: return "EQUALS";
    case SDL_SCANCODE_BACKSPACE: return "BACKSPACE";
    case SDL_SCANCODE_TAB: return "TAB";
    case SDL_SCANCODE_Q: return "Q";
    case SDL_SCANCODE_W: return "W";
    case SDL_SCANCODE_E: return "E";
    case SDL_SCANCODE_R: return "R";
    case SDL_SCANCODE_T: return "T";
    case SDL_SCANCODE_Y: return "Y";
    case SDL_SCANCODE_U: return "U";
    case SDL_SCANCODE_I: return "I";
    case SDL_SCANCODE_O: return "O";
    case SDL_SCANCODE_P: return "P";
    case SDL_SCANCODE_LEFTBRACKET: return "LEFTBRACKET";
    case SDL_SCANCODE_RIGHTBRACKET: return "RIGHTBRACKET";
    case SDL_SCANCODE_RETURN: return "ENTER";
    case SDL_SCANCODE_LCTRL: return "LCTRL";
    case SDL_SCANCODE_A: return "A";
    case SDL_SCANCODE_S: return "S";
    case SDL_SCANCODE_D: return "D";
    case SDL_SCANCODE_F: return "F";
    case SDL_SCANCODE_G: return "G";
    case SDL_SCANCODE_H: return "H";
    case SDL_SCANCODE_J: return "J";
    case SDL_SCANCODE_K: return "K";
    case SDL_SCANCODE_L: return "L";
    case SDL_SCANCODE_SEMICOLON: return "SEMICOLON";
    case SDL_SCANCODE_APOSTROPHE: return "APOSTROPHE";
    case SDL_SCANCODE_GRAVE: return "GRAVE";
    case SDL_SCANCODE_LSHIFT: return "LSHIFT";
    case SDL_SCANCODE_BACKSLASH: return "BACKSLASH";
    case SDL_SCANCODE_Z: return "Z";
    case SDL_SCANCODE_X: return "X";
    case SDL_SCANCODE_C: return "C";
    case SDL_SCANCODE_V: return "V";
    case SDL_SCANCODE_B: return "B";
    case SDL_SCANCODE_N: return "N";
    case SDL_SCANCODE_M: return "M";
    case SDL_SCANCODE_COMMA: return "COMMA";
    case SDL_SCANCODE_PERIOD: return "PERIOD";
    case SDL_SCANCODE_SLASH: return "SLASH";
    case SDL_SCANCODE_RSHIFT: return "RSHIFT";
    case SDL_SCANCODE_SPACE: return "SPACE";
    case SDL_SCANCODE_CAPSLOCK: return "CAPSLOCK";
    case SDL_SCANCODE_F1: return "F1";
    case SDL_SCANCODE_F2: return "F2";
    case SDL_SCANCODE_F4: return "F4";
    case SDL_SCANCODE_F5: return "F5";
    case SDL_SCANCODE_F6: return "F6";
    case SDL_SCANCODE_F7: return "F7";
    case SDL_SCANCODE_F12: return "F12";
    case SDL_SCANCODE_SCROLLLOCK: return "SCROLLLOCK";
    case SDL_SCANCODE_NUMLOCKCLEAR: return "NUMLOCK";
    case SDL_SCANCODE_INSERT: return "INSERT";
    case SDL_SCANCODE_HOME: return "HOME";
    case SDL_SCANCODE_PAGEUP: return "PAGEUP";
    case SDL_SCANCODE_DELETE: return "DELETE";
    case SDL_SCANCODE_END: return "END";
    case SDL_SCANCODE_PAGEDOWN: return "PAGEDOWN";
    case SDL_SCANCODE_RCTRL: return "RCTRL";
    case SDL_SCANCODE_LALT: return "LALT";
    case SDL_SCANCODE_RALT: return "RALT";
    case SDL_SCANCODE_APPLICATION: return "MENU";
    case SDL_SCANCODE_UP: return "UP";
    case SDL_SCANCODE_DOWN: return "DOWN";
    case SDL_SCANCODE_LEFT: return "LEFT";
    case SDL_SCANCODE_RIGHT: return "RIGHT";
    case SDL_SCANCODE_KP_DIVIDE: return "KP_DIVIDE";
    case SDL_SCANCODE_KP_MULTIPLY: return "KP_MULTIPLY";
    case SDL_SCANCODE_KP_MINUS: return "KP_MINUS";
    case SDL_SCANCODE_KP_PLUS: return "KP_PLUS";
    case SDL_SCANCODE_KP_ENTER: return "KP_ENTER";
    case SDL_SCANCODE_KP_1: return "KP_1";
    case SDL_SCANCODE_KP_2: return "KP_2";
    case SDL_SCANCODE_KP_3: return "KP_3";
    case SDL_SCANCODE_KP_4: return "KP_4";
    case SDL_SCANCODE_KP_5: return "KP_5";
    case SDL_SCANCODE_KP_6: return "KP_6";
    case SDL_SCANCODE_KP_7: return "KP_7";
    case SDL_SCANCODE_KP_8: return "KP_8";
    case SDL_SCANCODE_KP_9: return "KP_9";
    case SDL_SCANCODE_KP_0: return "KP_0";
    case SDL_SCANCODE_KP_PERIOD: return "KP_PERIOD";
    default:
      return "";
    }
  }

  uint64_t recordingTick(Machine &machine) const {
    return machine.tick() >= recording_start_tick_ ? machine.tick() - recording_start_tick_ : 0;
  }

  void recordLine(Machine &machine, const std::string &line, bool even_when_paused = false) {
    if (!recording_active_ || (recording_paused_ && !even_when_paused)) return;
    std::ostringstream out;
    out << "at " << recordingTick(machine) << " " << line;
    recording_lines_.push_back(out.str());
  }

  void recordKey(Machine &machine, const std::string &key, bool down) {
    recordLine(machine, "key " + key + (down ? " down" : " up"));
  }

  void recordMouse(Machine &machine, int dx, int dy, uint8_t buttons) {
    std::ostringstream out;
    out << "mouse " << dx << " " << dy << " " << static_cast<unsigned>(buttons);
    recordLine(machine, out.str());
  }

  void beginRecording(Machine &machine) {
    recording_active_ = true;
    recording_paused_ = false;
    recording_start_tick_ = machine.tick();
    recording_lines_.clear();
    record_status_ = "Recording";
    record_status_until_ns_ = SDL_GetTicksNS() + 1800000000ull;
  }

  void toggleRecordingPause(Machine &machine) {
    if (!recording_active_) return;
    recording_paused_ = !recording_paused_;
    recordLine(machine, recording_paused_ ? "capture out" : "capture in", true);
    record_status_ = recording_paused_ ? "Paused" : "Recording";
    record_status_until_ns_ = SDL_GetTicksNS() + 1800000000ull;
  }

  void finishRecording(Machine &machine) {
    if (!recording_active_) return;
    if (recording_paused_) {
      recording_paused_ = false;
      recordLine(machine, "capture in", true);
    }
    recording_active_ = false;
    save_modal_visible_ = true;
    mouse_capture_suppressed_ = true;
    setMouseCaptured(false);
    if (save_path_input_.empty()) {
      std::ostringstream path;
      path << "build/test-output/recording-" << machine.tick() << ".lcomscript";
      save_path_input_ = path.str();
    }
  }

  void handleSaveModalKey(const SDL_KeyboardEvent &key) {
    if (key.scancode == SDL_SCANCODE_ESCAPE) {
      save_modal_visible_ = false;
      record_status_ = "Recording discarded";
      record_status_until_ns_ = SDL_GetTicksNS() + 2200000000ull;
      return;
    }
    if (key.scancode == SDL_SCANCODE_RETURN) {
      saveRecording();
      return;
    }
    if (key.scancode == SDL_SCANCODE_BACKSPACE) {
      if (!save_path_input_.empty()) save_path_input_.pop_back();
      return;
    }
    SDL_Keycode sym = key.key;
    if (sym >= 32 && sym <= 126) {
      save_path_input_.push_back(static_cast<char>(sym));
    }
  }

  void saveRecording() {
    std::filesystem::path path = save_path_input_.empty()
                                     ? std::filesystem::path("lcom-recording.lcomscript")
                                     : std::filesystem::path(save_path_input_);
    std::error_code ec;
    if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path(), ec);
    std::ofstream out(path);
    if (out.is_open()) {
      out << "# lcom-ng recording\n";
      out << "# F8 writes capture out/in markers; skipped frames are intentionally absent from videos.\n";
      out << "# Ticks are virtual timer ticks; use `caption top 2s Text` or `caption bottom 2s Text`.\n";
      for (const auto &line : recording_lines_) out << line << "\n";
      save_modal_visible_ = false;
      record_status_ = "Saved script";
      record_status_until_ns_ = SDL_GetTicksNS() + 2600000000ull;
    } else {
      record_status_ = "Save failed";
      record_status_until_ns_ = SDL_GetTicksNS() + 2600000000ull;
    }
  }

  SDL_Window *window_ = nullptr;
  SDL_Renderer *renderer_ = nullptr;
  SDL_Texture *texture_ = nullptr;
  TTF_Font *font_ = nullptr;
  TTF_Font *title_font_ = nullptr;
  Machine *observed_machine_ = nullptr;
  uint32_t window_id_ = 0;

  SdlBackendOptions options_{};
  bool sdl_acquired_ = false;
  bool debug_visible_ = false;
  bool fullscreen_ = false;
  bool should_quit_ = false;
  bool mouse_captured_ = false;
  bool mouse_capture_suppressed_ = false;
  bool drop_next_relative_sample_ = false;
  uint8_t host_mouse_buttons_ = 0;
  MousePacketScheduler mouse_scheduler_;
  uint64_t last_mouse_sample_ns_ = 0;
  uint64_t startup_hint_until_ns_ = 0;
  uint64_t last_present_ns_ = 0;
  float current_frame_ms_ = 0.0f;
  float current_fps_ = 0.0f;
  int logical_width_ = 800;
  int logical_height_ = 600;
  int texture_width_ = 0;
  int texture_height_ = 0;
  SDL_PixelFormat texture_format_ = SDL_PIXELFORMAT_UNKNOWN;

  std::vector<std::string> terminal_lines_;
  std::string current_line_;
  std::vector<uint8_t> rgb_buffer_;
  std::vector<float> frame_ms_samples_;
  std::vector<std::string> scancode_history_;
  std::vector<std::string> mouse_packet_history_;
  SDL_FRect record_button_{};
  SDL_FRect pause_button_{};
  SDL_FRect stop_button_{};
  bool recording_active_ = false;
  bool recording_paused_ = false;
  bool save_modal_visible_ = false;
  uint64_t recording_start_tick_ = 0;
  uint64_t record_status_until_ns_ = 0;
  std::string record_status_;
  std::string save_path_input_;
  std::vector<std::string> recording_lines_;
};

std::unique_ptr<DisplayBackend> createSdlBackend(const SdlBackendOptions &options) {
  return std::unique_ptr<DisplayBackend>(new SdlBackend(options));
}

} // namespace lcom

#endif
