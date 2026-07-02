#include "RuntimeServer.hpp"

#include "lcom_protocol.h"
#include "../backends/HeadlessDisplay.hpp"
#if defined(MACHINE_LAB_WITH_SDL)
#include "../backends/SdlAudioBackend.hpp"
#include "../backends/SdlBackend.hpp"
#endif

#include <lcom/ac97.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <csignal>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <filesystem>
#include <fcntl.h>
#include <iostream>
#include <sstream>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace lcom {

static constexpr size_t kFramebufferShmBytes = 1280u * 1024u * 4u;
static constexpr size_t kAudioShmOffset = kFramebufferShmBytes;
static constexpr int kMaxRealtimeTicksPerLoop = 4;
static constexpr auto kDisplayPumpInterval = std::chrono::nanoseconds(1000000000ull / 120ull);

static int writeAll(int fd, const void *buf, size_t len) {
  const uint8_t *p = static_cast<const uint8_t *>(buf);
  while (len > 0) {
    ssize_t n = write(fd, p, len);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (n == 0) return -1;
    p += static_cast<size_t>(n);
    len -= static_cast<size_t>(n);
  }
  return 0;
}

static int readAll(int fd, void *buf, size_t len) {
  uint8_t *p = static_cast<uint8_t *>(buf);
  while (len > 0) {
    ssize_t n = read(fd, p, len);
    if (n < 0) {
      if (errno == EINTR) continue;
      return -1;
    }
    if (n == 0) return -1;
    p += static_cast<size_t>(n);
    len -= static_cast<size_t>(n);
  }
  return 0;
}

static bool sendMessage(int fd, uint16_t type, uint32_t request_id,
                        const void *payload, uint32_t size) {
  lcom_msg_header_t hdr{};
  hdr.type = type;
  hdr.flags = 0;
  hdr.size = size;
  hdr.request_id = request_id;
  if (writeAll(fd, &hdr, sizeof(hdr)) != 0) return false;
  if (size != 0 && writeAll(fd, payload, size) != 0) return false;
  return true;
}

static void setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

RuntimeServer::RuntimeServer(RuntimeOptions options) : options_(std::move(options)) {}

RuntimeServer::~RuntimeServer() {
  cleanupChild();
  cleanupSharedMemory();
  cleanupVideoCapture();
}

int RuntimeServer::run() {
  if (!setup()) return 1;
  if (!startChild()) return 1;

  using Clock = std::chrono::steady_clock;
  auto realtimeTickInterval = [&]() {
    uint32_t hz = machine_.pit().channelFrequency(0);
    if (hz == 0) hz = 60;
    uint64_t ns = 1000000000ull / hz;
    if (ns < 1000000ull) ns = 1000000ull;
    return std::chrono::nanoseconds(ns);
  };
  auto next_realtime_tick = Clock::now() + realtimeTickInterval();
  auto next_display_pump = Clock::now();
  auto advanceRealtimeIfDue = [&]() {
    if (!options_.realtime) return;
    auto now = Clock::now();
    int advanced = 0;
    while (now >= next_realtime_tick && child_running_) {
      advanceVirtualTimeOnce();
      advanced++;
      auto interval = realtimeTickInterval();
      next_realtime_tick += interval;
      if (advanced >= kMaxRealtimeTicksPerLoop && now >= next_realtime_tick) {
        next_realtime_tick = now + interval;
        break;
      }
    }
    if (advanced > 0) maybeSatisfyEventWait();
  };
  auto pumpDisplayIfDue = [&]() {
    if (display_ == nullptr) return;
    if (options_.realtime) {
      auto now = Clock::now();
      if (now < next_display_pump) return;
      next_display_pump = now + kDisplayPumpInterval;
    }
    display_->pump(machine_);
    maybeSatisfyEventWait();
  };
  auto selectTimeout = [&]() {
    timeval tv{};
    if (options_.realtime) {
      auto now = Clock::now();
      auto next_wake = next_realtime_tick;
      if (display_ != nullptr && next_display_pump < next_wake) next_wake = next_display_pump;
      auto wait = next_wake > now
                      ? std::chrono::duration_cast<std::chrono::microseconds>(next_wake - now)
                      : std::chrono::microseconds(0);
      if (wait > std::chrono::microseconds(16000)) wait = std::chrono::microseconds(16000);
      tv.tv_sec = static_cast<long>(wait.count() / 1000000);
      tv.tv_usec = static_cast<int>(wait.count() % 1000000);
    } else {
      tv.tv_sec = 0;
      tv.tv_usec = 10000;
    }
    return tv;
  };

  while (child_running_ || client_fd_ >= 0) {
    advanceRealtimeIfDue();
    pumpDisplayIfDue();
    maybeSatisfyEventWait();

    if (waiting_event_ && !options_.realtime) {
      advanceVirtualTimeOnce();
      maybeSatisfyEventWait();
      if (!childExited()) continue;
    }

    fd_set readfds;
    FD_ZERO(&readfds);
    int max_fd = -1;
    auto add_fd = [&](int fd) {
      if (fd >= 0) {
        FD_SET(fd, &readfds);
        if (fd > max_fd) max_fd = fd;
      }
    };
    add_fd(client_fd_);
    add_fd(child_stdout_);
    add_fd(child_stderr_);

    timeval tv = selectTimeout();
    int ready = max_fd >= 0 ? select(max_fd + 1, &readfds, nullptr, nullptr, &tv) : 0;
    if (ready < 0 && errno != EINTR) {
      perror("select");
      break;
    }

    pumpDisplayIfDue();

    if (client_fd_ >= 0 && FD_ISSET(client_fd_, &readfds)) {
      if (!handleClientMessage()) {
        close(client_fd_);
        client_fd_ = -1;
      }
    }
    if (child_stdout_ >= 0 && FD_ISSET(child_stdout_, &readfds)) drainPipe(child_stdout_, "stdout");
    if (child_stderr_ >= 0 && FD_ISSET(child_stderr_, &readfds)) drainPipe(child_stderr_, "stderr");

    advanceRealtimeIfDue();

    if (display_ != nullptr && !machine_.vbe().graphicsMode()) display_->present(machine_);

    childExited();
  }

  if (!options_.dump_frame_path.empty()) {
    syncFramebufferFromSharedMemory();
    machine_.vbe().dumpPpm(options_.dump_frame_path, caption_text_, caption_position_);
  }

  return renderVideo() ? child_exit_status_ : 1;
}

bool RuntimeServer::setup() {
  if (!options_.trace_path.empty()) {
    if (!trace_.open(options_.trace_path)) {
      std::cerr << "machinelab: could not open trace file " << options_.trace_path << "\n";
      return false;
    }
    machine_.setTrace(&trace_);
  }

  if (!options_.script_path.empty()) {
    std::string error;
    if (!script_.load(options_.script_path, error)) {
      std::cerr << "machinelab: " << error << "\n";
      return false;
    }
  }

  if (!options_.rtc_time.empty()) {
    if (!machine_.rtc().setIsoTime(options_.rtc_time)) {
      std::cerr << "machinelab: invalid RTC time '" << options_.rtc_time << "'\n";
      return false;
    }
  }

  if (!setupSharedMemory()) return false;

  if (!setupDisplay()) return false;
  if (!setupAudio()) return false;
  if (!setupVideoCapture()) return false;

  return true;
}

bool RuntimeServer::setupDisplay() {
  if (options_.display == "headless") {
    display_.reset(new HeadlessDisplay());
  } else if (options_.display == "sdl") {
#if defined(MACHINE_LAB_WITH_SDL)
    SdlBackendOptions sdl_options;
    sdl_options.fullscreen = options_.fullscreen;
    sdl_options.integer_scale = options_.integer_scale;
    sdl_options.guest_input = options_.guest_input;
    sdl_options.scale = options_.scale;
    display_ = createSdlBackend(sdl_options);
#else
    std::cerr << "machinelab: SDL backend requested but this build was compiled without SDL3\n";
    return false;
#endif
  } else {
    std::cerr << "machinelab: unknown display backend '" << options_.display << "'\n";
    return false;
  }

  if (display_ != nullptr && !display_->start(machine_)) {
    std::cerr << "machinelab: display backend failed to start\n";
    return false;
  }

  return true;
}

bool RuntimeServer::setupAudio() {
  if (!options_.audio_wav_path.empty()) {
    audio_.reset(new WavAudioBackend(options_.audio_wav_path));
  } else if (options_.audio == "null") {
    audio_.reset(new NullAudioBackend());
  } else if (options_.audio == "sdl") {
#if defined(MACHINE_LAB_WITH_SDL)
    audio_ = createSdlAudioBackend();
#else
    std::cerr << "machinelab: SDL audio requested but this build was compiled without SDL3\n";
    return false;
#endif
  } else if (options_.audio.rfind("wav:", 0) == 0) {
    audio_.reset(new WavAudioBackend(options_.audio.substr(4)));
  } else {
    std::cerr << "machinelab: unknown audio backend '" << options_.audio << "'\n";
    return false;
  }

  std::string error;
  if (audio_ != nullptr && !audio_->start(error)) {
    std::cerr << "machinelab: audio backend failed to start";
    if (!error.empty()) std::cerr << ": " << error;
    std::cerr << "\n";
    return false;
  }
  return true;
}

bool RuntimeServer::setupSharedMemory() {
  shm_size_ = kAudioShmOffset + machine_.ac97().bufferSize();
  std::ostringstream name;
  name << "/machine-lab-" << getpid() << "-" << std::rand();
  shm_name_ = name.str();

  shm_fd_ = shm_open(shm_name_.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
  if (shm_fd_ < 0) {
    perror("shm_open");
    return false;
  }
  if (ftruncate(shm_fd_, static_cast<off_t>(shm_size_)) != 0) {
    perror("ftruncate");
    return false;
  }
  void *mapped = mmap(nullptr, shm_size_, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_, 0);
  if (mapped == MAP_FAILED) {
    perror("mmap");
    return false;
  }
  shm_data_ = static_cast<uint8_t *>(mapped);
  zeroSharedFramebuffer();
  return true;
}

bool RuntimeServer::startChild() {
  if (options_.program.empty()) {
    std::cerr << "machinelab: no program specified\n";
    return false;
  }

  int sv[2];
  if (socketpair(AF_UNIX, SOCK_STREAM, 0, sv) != 0) {
    perror("socketpair");
    return false;
  }

  int out_pipe[2];
  int err_pipe[2];
  if (pipe(out_pipe) != 0 || pipe(err_pipe) != 0) {
    perror("pipe");
    return false;
  }

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return false;
  }

  if (pid == 0) {
    close(sv[0]);
    close(out_pipe[0]);
    close(err_pipe[0]);
    dup2(out_pipe[1], STDOUT_FILENO);
    dup2(err_pipe[1], STDERR_FILENO);
    close(out_pipe[1]);
    close(err_pipe[1]);

    char fd_buf[32];
    std::snprintf(fd_buf, sizeof(fd_buf), "%d", sv[1]);
    setenv("MACHINE_LAB_RUN_FD", fd_buf, 1);
    setenv("LCOM_RUN_FD", fd_buf, 1);

    std::vector<char *> argv;
    argv.reserve(options_.program.size() + 1);
    for (std::string &arg : options_.program) {
      argv.push_back(const_cast<char *>(arg.c_str()));
    }
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    perror("execvp");
    _exit(127);
  }

  close(sv[1]);
  close(out_pipe[1]);
  close(err_pipe[1]);

  client_fd_ = sv[0];
  child_stdout_ = out_pipe[0];
  child_stderr_ = err_pipe[0];
  setNonBlocking(child_stdout_);
  setNonBlocking(child_stderr_);
  child_pid_ = static_cast<int>(pid);
  child_running_ = true;

  std::cout << "Starting Machine Lab bus instance...\n"
            << "  PIT8254    ports 0x40-0x43  irq 0\n"
            << "  i8042      ports 0x60/0x64  irq 1/12\n"
            << "  RTC-CMOS   ports 0x70/0x71  irq 8\n"
            << "  UART16550  ports 0x3F8/0x2F8 irq 4/3\n"
            << "  AC97-lite  ports 0xC000/0xC100 pcm 0xD0000000\n"
            << "  VBE        framebuffer 0xE0000000\n"
            << "Running " << options_.program[0] << "...\n";
  return true;
}

void RuntimeServer::cleanupChild() {
  if (client_fd_ >= 0) {
    close(client_fd_);
    client_fd_ = -1;
  }
  if (child_stdout_ >= 0) {
    close(child_stdout_);
    child_stdout_ = -1;
  }
  if (child_stderr_ >= 0) {
    close(child_stderr_);
    child_stderr_ = -1;
  }
  if (child_running_ && child_pid_ > 0) {
    kill(child_pid_, SIGTERM);
    waitpid(child_pid_, nullptr, 0);
    child_running_ = false;
  }
}

void RuntimeServer::cleanupSharedMemory() {
  if (shm_data_ != nullptr) {
    munmap(shm_data_, shm_size_);
    shm_data_ = nullptr;
  }
  if (shm_fd_ >= 0) {
    close(shm_fd_);
    shm_fd_ = -1;
  }
  if (!shm_name_.empty()) {
    shm_unlink(shm_name_.c_str());
    shm_name_.clear();
  }
}

bool RuntimeServer::handleClientMessage() {
  lcom_msg_header_t hdr{};
  if (readAll(client_fd_, &hdr, sizeof(hdr)) != 0) return false;
  if (hdr.size > LCOM_MAX_PAYLOAD) return false;

  uint8_t payload[LCOM_MAX_PAYLOAD];
  if (hdr.size != 0 && readAll(client_fd_, payload, hdr.size) != 0) return false;

  switch (hdr.type) {
  case LCOM_MSG_HELLO: {
    lcom_hello_reply_t reply{};
    reply.status = 0;
    reply.version = LCOM_PROTOCOL_VERSION;
    sendMessage(client_fd_, LCOM_MSG_HELLO_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_EXIT:
    return false;
  case LCOM_MSG_CONSOLE_WRITE:
    handleConsoleWrite(reinterpret_cast<const char *>(payload), hdr.size);
    return true;
  case LCOM_MSG_PORT_READ8: {
    if (hdr.size != sizeof(lcom_port_read8_t)) return false;
    auto *req = reinterpret_cast<lcom_port_read8_t *>(payload);
    uint8_t value = 0;
    bool ok = machine_.readPort8(req->port, value);
    lcom_port_read8_reply_t reply{};
    reply.status = ok ? 0 : -1;
    reply.value = value;
    sendMessage(client_fd_, LCOM_MSG_PORT_READ8_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_PORT_WRITE8: {
    if (hdr.size != sizeof(lcom_port_write8_t)) return false;
    auto *req = reinterpret_cast<lcom_port_write8_t *>(payload);
    bool was_audio_playing = machine_.ac97().playing();
    bool force_audio_play = req->port == AC97_BM_BASE + AC97_PO_CR &&
                            (req->value & AC97_PO_CR_RUN) != 0;
    bool ok = machine_.writePort8(req->port, req->value);
    if (ok) ok = updateAudioBackendFromDevice(was_audio_playing, force_audio_play);
    sendStatus(hdr.request_id, ok ? 0 : -1);
    return true;
  }
  case LCOM_MSG_IRQ_SUBSCRIBE: {
    if (hdr.size != sizeof(lcom_irq_subscribe_t)) return false;
    auto *req = reinterpret_cast<lcom_irq_subscribe_t *>(payload);
    IrqSubscription sub;
    bool ok = machine_.subscribeIrq(req->irq, sub);
    lcom_irq_subscribe_reply_t reply{};
    reply.status = ok ? 0 : -1;
    reply.irq = sub.irq;
    reply.bit_no = sub.bit_no;
    reply.mask = sub.mask;
    sendMessage(client_fd_, LCOM_MSG_IRQ_SUBSCRIBE_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_IRQ_UNSUBSCRIBE: {
    if (hdr.size != sizeof(lcom_irq_unsubscribe_t)) return false;
    auto *req = reinterpret_cast<lcom_irq_unsubscribe_t *>(payload);
    sendStatus(hdr.request_id, machine_.unsubscribeIrq(req->irq) ? 0 : -1);
    return true;
  }
  case LCOM_MSG_EVENT_WAIT:
    waiting_event_ = true;
    waiting_request_id_ = hdr.request_id;
    maybeSatisfyEventWait();
    return true;
  case LCOM_MSG_PHYS_MAP: {
    if (hdr.size != sizeof(lcom_phys_map_t)) return false;
    auto *req = reinterpret_cast<lcom_phys_map_t *>(payload);
    lcom_phys_map_reply_t reply{};
    if (machine_.vbe().ownsRange(req->phys, req->length) && req->length <= kFramebufferShmBytes) {
      reply.status = 0;
      std::snprintf(reply.shm_name, sizeof(reply.shm_name), "%s", shm_name_.c_str());
      reply.offset = req->phys - machine_.vbe().framebufferPhys();
      reply.length = req->length;
    } else if (machine_.ac97().ownsRange(req->phys, req->length) &&
               kAudioShmOffset + req->length <= shm_size_) {
      reply.status = 0;
      std::snprintf(reply.shm_name, sizeof(reply.shm_name), "%s", shm_name_.c_str());
      reply.offset = kAudioShmOffset + (req->phys - machine_.ac97().bufferPhys());
      reply.length = req->length;
    } else {
      reply.status = -1;
    }
    sendMessage(client_fd_, LCOM_MSG_PHYS_MAP_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_VBE_GET_MODE_INFO: {
    if (hdr.size != sizeof(lcom_vbe_mode_request_t)) return false;
    auto *req = reinterpret_cast<lcom_vbe_mode_request_t *>(payload);
    VbeModeInfo info;
    bool ok = machine_.vbe().modeInfo(req->mode, info);
    lcom_vbe_mode_info_wire_t reply{};
    reply.status = ok ? 0 : -1;
    reply.mode = info.mode;
    reply.width = info.width;
    reply.height = info.height;
    reply.bpp = info.bpp;
    reply.bytes_per_pixel = info.bytes_per_pixel;
    reply.pitch = info.pitch;
    reply.framebuffer_phys = info.framebuffer_phys;
    reply.framebuffer_size = info.framebuffer_size;
    sendMessage(client_fd_, LCOM_MSG_VBE_MODE_INFO_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_VBE_SET_MODE: {
    if (hdr.size != sizeof(lcom_vbe_mode_request_t)) return false;
    auto *req = reinterpret_cast<lcom_vbe_mode_request_t *>(payload);
    bool ok = machine_.vbe().setMode(req->mode);
    zeroSharedFramebuffer();
    sendStatus(hdr.request_id, ok ? 0 : -1);
    return true;
  }
  case LCOM_MSG_VBE_PRESENT:
    syncFramebufferFromSharedMemory();
    if (!options_.dump_frame_path.empty()) {
      machine_.vbe().dumpPpm(options_.dump_frame_path, caption_text_, caption_position_);
    }
    dumpVideoFrame();
    if (display_ != nullptr) display_->present(machine_);
    sendStatus(hdr.request_id, 0);
    return true;
  case LCOM_MSG_AC97_GET_BUFFER: {
    lcom_ac97_buffer_wire_t reply{};
    reply.status = 0;
    reply.pcm_phys = machine_.ac97().bufferPhys();
    reply.pcm_bytes = machine_.ac97().bufferSize();
    reply.sample_rate = machine_.ac97().sampleRate();
    reply.channels = machine_.ac97().channels();
    reply.bits_per_sample = 16;
    sendMessage(client_fd_, LCOM_MSG_AC97_BUFFER_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  default:
    return false;
  }
}

bool RuntimeServer::updateAudioBackendFromDevice(bool was_playing, bool force_play) {
  bool now_playing = machine_.ac97().playing();
  if (!force_play && was_playing == now_playing) return true;

  if (!now_playing) {
    if (audio_backend_playing_ && audio_ != nullptr) audio_->stop();
    audio_backend_playing_ = false;
    return true;
  }

  syncAudioFromSharedMemory();
  if (audio_backend_playing_ && audio_ != nullptr) audio_->stop();
  std::string error;
  size_t frames = machine_.ac97().playByteCount() /
                  (sizeof(int16_t) * machine_.ac97().channels());
  bool ok = audio_ == nullptr ||
            audio_->playPcm16(reinterpret_cast<const int16_t *>(machine_.ac97().pcm().data()),
                              frames,
                              machine_.ac97().sampleRate(),
                              machine_.ac97().channels(),
                              error);
  audio_backend_playing_ = ok;
  if (!ok && !error.empty()) std::cerr << "machinelab: audio play failed: " << error << "\n";
  return ok;
}

void RuntimeServer::handleConsoleWrite(const char *data, size_t size) {
  std::cout.write(data, static_cast<std::streamsize>(size));
  std::cout.flush();
  if (display_ != nullptr) display_->consoleWrite(data, size);
}

void RuntimeServer::sendStatus(uint32_t request_id, int32_t status) {
  lcom_status_reply_t reply{};
  reply.status = status;
  sendMessage(client_fd_, LCOM_MSG_STATUS, request_id, &reply, sizeof(reply));
}

void RuntimeServer::sendEventReply(uint32_t request_id, uint32_t irq_mask) {
  lcom_event_reply_t reply{};
  reply.status = 0;
  reply.irq_mask = irq_mask;
  reply.tick = machine_.tick();
  sendMessage(client_fd_, LCOM_MSG_EVENT_REPLY, request_id, &reply, sizeof(reply));
}

void RuntimeServer::maybeSatisfyEventWait() {
  if (!waiting_event_) return;
  machine_.refreshDeviceIrqs();
  uint32_t pending = machine_.pendingIrqs();
  if (pending == 0) return;
  uint32_t mask = machine_.consumePendingIrqs();
  waiting_event_ = false;
  sendEventReply(waiting_request_id_, mask);
}

void RuntimeServer::advanceVirtualTimeOnce() {
  if (machine_.tick() >= options_.max_ticks) {
    std::cerr << "machinelab: max virtual ticks reached (" << options_.max_ticks << ")\n";
    child_exit_status_ = 1;
    cleanupChild();
    return;
  }
  machine_.advanceTick();
  applyScriptEvents(script_.takeDue(machine_.tick()));
}

void RuntimeServer::applyScriptEvents(const std::vector<ScriptEvent> &events) {
  for (const ScriptEvent &ev : events) {
    switch (ev.kind) {
    case ScriptEvent::Kind::Key:
      machine_.injectKey(ev.key, ev.pressed);
      break;
    case ScriptEvent::Kind::Mouse:
      machine_.injectMouse(ev.dx, ev.dy, ev.buttons);
      break;
    case ScriptEvent::Kind::Text:
      for (char c : ev.text) {
        if (c >= 'a' && c <= 'z') {
          std::string key(1, static_cast<char>(std::toupper(static_cast<unsigned char>(c))));
          machine_.injectKey(key, true);
          machine_.injectKey(key, false);
        } else if (c >= 'A' && c <= 'Z') {
          std::string key(1, c);
          machine_.injectKey(key, true);
          machine_.injectKey(key, false);
        } else if (c == ' ') {
          machine_.injectKey("SPACE", true);
          machine_.injectKey("SPACE", false);
        }
      }
      break;
    case ScriptEvent::Kind::Rtc:
      machine_.rtc().setIsoTime(ev.text);
      break;
    case ScriptEvent::Kind::Caption:
      caption_text_ = ev.text;
      caption_position_ = ev.caption_position;
      caption_until_tick_ = machine_.tick() + ev.duration;
      break;
    case ScriptEvent::Kind::Capture:
      video_capture_enabled_ = ev.capture_enabled;
      break;
    case ScriptEvent::Kind::Tick:
      break;
    }
  }
}

void RuntimeServer::syncFramebufferFromSharedMemory() {
  Vbe &vbe = machine_.vbe();
  size_t n = std::min(vbe.framebuffer().size(), kFramebufferShmBytes);
  if (shm_data_ != nullptr && n != 0) {
    std::copy(shm_data_, shm_data_ + n, vbe.framebuffer().begin());
  }
}

void RuntimeServer::syncAudioFromSharedMemory() {
  Ac97 &ac97 = machine_.ac97();
  size_t requested = ac97.playByteCount() == 0 ? ac97.bufferSize() : ac97.playByteCount();
  size_t n = std::min(requested, std::min(ac97.bufferSize(), shm_size_ - kAudioShmOffset));
  if (shm_data_ != nullptr && n != 0) {
    std::copy(shm_data_ + kAudioShmOffset, shm_data_ + kAudioShmOffset + n, ac97.pcm().begin());
  }
}

void RuntimeServer::zeroSharedFramebuffer() {
  if (shm_data_ != nullptr && shm_size_ != 0) {
    std::memset(shm_data_, 0, shm_size_);
  }
}

static bool isFramePpmName(const std::filesystem::path &path) {
  std::string name = path.filename().string();
  return name.rfind("frame_", 0) == 0 && path.extension() == ".ppm";
}

bool RuntimeServer::setupVideoCapture() {
  if (options_.frame_dir_path.empty() && options_.video_path.empty()) return true;
  capture_frame_stride_ = 1;
  if (!options_.video_path.empty() && options_.video_fps > 0 && options_.video_fps < 60) {
    capture_frame_stride_ = std::max<uint32_t>(1, 60 / options_.video_fps);
  }

  if (!options_.frame_dir_path.empty()) {
    active_frame_dir_ = options_.frame_dir_path;
    remove_frame_dir_ = false;
  } else {
    std::ostringstream dir;
    dir << "/tmp/machine-lab-video-" << getpid() << "-" << std::rand();
    active_frame_dir_ = dir.str();
    remove_frame_dir_ = true;
  }

  std::error_code ec;
  std::filesystem::create_directories(active_frame_dir_, ec);
  if (ec) {
    std::cerr << "machinelab: could not create frame directory " << active_frame_dir_
              << ": " << ec.message() << "\n";
    return false;
  }

  for (const auto &entry : std::filesystem::directory_iterator(active_frame_dir_, ec)) {
    if (!ec && entry.is_regular_file() && isFramePpmName(entry.path())) {
      std::filesystem::remove(entry.path(), ec);
    }
  }

  if (!options_.video_path.empty()) {
    std::filesystem::path video(options_.video_path);
    if (video.has_parent_path()) {
      std::filesystem::create_directories(video.parent_path(), ec);
      if (ec) {
        std::cerr << "machinelab: could not create video directory "
                  << video.parent_path().string() << ": " << ec.message() << "\n";
        return false;
      }
    }
  }

  return true;
}

void RuntimeServer::dumpVideoFrame() {
  if (active_frame_dir_.empty() || !video_capture_enabled_) return;
  presented_frame_index_++;
  if (capture_frame_stride_ > 1 &&
      ((presented_frame_index_ - 1) % capture_frame_stride_) != 0) {
    return;
  }
  if (caption_until_tick_ != 0 && machine_.tick() >= caption_until_tick_) {
    caption_text_.clear();
    caption_until_tick_ = 0;
  }
  frame_index_++;
  char filename[64];
  std::snprintf(filename, sizeof(filename), "frame_%06u.ppm", frame_index_);
  std::filesystem::path path = std::filesystem::path(active_frame_dir_) / filename;
  machine_.vbe().dumpPpm(path.string(), caption_text_, caption_position_);
}

bool RuntimeServer::renderVideo() {
  if (options_.video_path.empty()) return true;
  if (frame_index_ == 0) {
    std::cerr << "machinelab: no frames were presented; cannot render video\n";
    return false;
  }

  std::string fps = std::to_string(options_.video_fps == 0 ? 60 : options_.video_fps);
  std::string pattern = (std::filesystem::path(active_frame_dir_) / "frame_%06d.ppm").string();

  pid_t pid = fork();
  if (pid < 0) {
    perror("fork");
    return false;
  }
  if (pid == 0) {
    execlp("ffmpeg", "ffmpeg", "-y", "-hide_banner", "-loglevel", "error",
           "-framerate", fps.c_str(), "-i", pattern.c_str(),
           "-pix_fmt", "yuv420p", options_.video_path.c_str(), nullptr);
    perror("ffmpeg");
    _exit(127);
  }

  int status = 0;
  if (waitpid(pid, &status, 0) < 0) {
    perror("waitpid");
    return false;
  }
  if (!WIFEXITED(status) || WEXITSTATUS(status) != 0) {
    std::cerr << "machinelab: ffmpeg failed while rendering " << options_.video_path << "\n";
    return false;
  }

  std::cout << "Rendered video " << options_.video_path << " from "
            << frame_index_ << " frames\n";
  cleanupVideoCapture();
  return true;
}

void RuntimeServer::cleanupVideoCapture() {
  if (remove_frame_dir_ && !active_frame_dir_.empty()) {
    std::error_code ec;
    std::filesystem::remove_all(active_frame_dir_, ec);
  }
  active_frame_dir_.clear();
  remove_frame_dir_ = false;
}

void RuntimeServer::drainPipe(int fd, const char *label) {
  char buf[1024];
  for (;;) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
      if (std::strcmp(label, "stderr") == 0) {
        std::cerr.write(buf, n);
        std::cerr.flush();
      } else {
        std::cout.write(buf, n);
        std::cout.flush();
      }
      if (display_ != nullptr) display_->consoleWrite(buf, static_cast<size_t>(n));
      continue;
    }
    if (n == 0) {
      close(fd);
      if (fd == child_stdout_) child_stdout_ = -1;
      if (fd == child_stderr_) child_stderr_ = -1;
      return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return;
    return;
  }
}

bool RuntimeServer::childExited() {
  if (!child_running_ || child_pid_ <= 0) return true;
  int status = 0;
  pid_t ret = waitpid(child_pid_, &status, WNOHANG);
  if (ret == 0) return false;
  if (ret == child_pid_) {
    child_running_ = false;
    child_pid_ = -1;
    if (WIFEXITED(status)) {
      child_exit_status_ = WEXITSTATUS(status);
      std::cout << "Program exited with status " << WEXITSTATUS(status) << "\n";
    } else if (WIFSIGNALED(status)) {
      child_exit_status_ = 128 + WTERMSIG(status);
      std::cout << "Program terminated by signal " << WTERMSIG(status) << "\n";
    }
    if (client_fd_ >= 0) {
      close(client_fd_);
      client_fd_ = -1;
    }
    return true;
  }
  return false;
}

} // namespace lcom
