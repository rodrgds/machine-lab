#include "PairRuntimeServer.hpp"
#include "ProtocolIO.hpp"

#include "lcom_protocol.h"
#include "../backends/HeadlessDisplay.hpp"
#if defined(MACHINE_LAB_WITH_SDL)
#include "../backends/SdlBackend.hpp"
#endif

#include <lcom/ac97.h>

#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cctype>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fcntl.h>
#include <iostream>
#include <signal.h>
#include <sstream>
#include <utility>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

namespace lcom {

namespace {

static constexpr size_t kFramebufferShmBytes = 1280u * 1024u * 4u;
static constexpr size_t kAudioShmOffset = kFramebufferShmBytes;
static constexpr int kMaxRealtimeTicksPerLoop = 4;
static constexpr auto kDisplayPumpInterval = std::chrono::nanoseconds(1000000000ull / 120ull);

void setNonBlocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (flags >= 0) fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

} // namespace

PairRuntimeServer::PairRuntimeServer(PairRuntimeOptions options)
    : options_(std::move(options)) {
  left_.name = "left";
  left_.program = options_.left_program;
  left_.script_path = options_.left_script_path;
  right_.name = "right";
  right_.program = options_.right_program;
  right_.script_path = options_.right_script_path;
}

PairRuntimeServer::~PairRuntimeServer() {
  cleanupSlot(left_);
  cleanupSlot(right_);
}

int PairRuntimeServer::run() {
  if (!setup()) return 1;
  int runtime_status = 0;

  std::cout << "Starting paired Machine Lab bus instances...\n"
            << "  left  COM1/COM2 bridged to right COM1/COM2\n"
            << "  right COM1/COM2 bridged to left COM1/COM2\n";

  using Clock = std::chrono::steady_clock;
  auto realtimeTickInterval = [&]() {
    uint32_t hz = left_.machine.pit().channelFrequency(0);
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
    while (now >= next_realtime_tick && anyAlive()) {
      advanceAllOnce();
      advanced++;
      auto interval = realtimeTickInterval();
      next_realtime_tick += interval;
      if (advanced >= kMaxRealtimeTicksPerLoop && now >= next_realtime_tick) {
        next_realtime_tick = now + interval;
        break;
      }
    }
    if (advanced > 0) {
      maybeSatisfyEventWait(left_);
      maybeSatisfyEventWait(right_);
    }
  };
  auto pumpDisplaysIfDue = [&]() {
    if (left_.display == nullptr && right_.display == nullptr) return;
    if (options_.realtime) {
      auto now = Clock::now();
      if (now < next_display_pump) return;
      next_display_pump = now + kDisplayPumpInterval;
    }
    if (left_.display != nullptr) left_.display->pump(left_.machine);
    if (right_.display != nullptr) right_.display->pump(right_.machine);
    maybeSatisfyEventWait(left_);
    maybeSatisfyEventWait(right_);
  };
  auto selectTimeout = [&]() {
    timeval tv{};
    if (options_.realtime) {
      auto now = Clock::now();
      auto next_wake = next_realtime_tick;
      if ((left_.display != nullptr || right_.display != nullptr) && next_display_pump < next_wake) {
        next_wake = next_display_pump;
      }
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

  while (anyAlive()) {
    advanceRealtimeIfDue();
    pumpDisplaysIfDue();
    maybeSatisfyEventWait(left_);
    maybeSatisfyEventWait(right_);

    if (!options_.realtime && shouldAdvanceHeadless()) {
      advanceAllOnce();
      maybeSatisfyEventWait(left_);
      maybeSatisfyEventWait(right_);
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
    add_fd(left_.client_fd);
    add_fd(left_.child_stdout);
    add_fd(left_.child_stderr);
    add_fd(right_.client_fd);
    add_fd(right_.child_stdout);
    add_fd(right_.child_stderr);

    timeval tv = selectTimeout();
    int ready = max_fd >= 0 ? select(max_fd + 1, &readfds, nullptr, nullptr, &tv) : 0;
    if (ready < 0 && errno != EINTR) {
      perror("select");
      runtime_status = 1;
      break;
    }

    pumpDisplaysIfDue();

    auto service = [&](Slot &slot) {
      if (slot.client_fd >= 0 && FD_ISSET(slot.client_fd, &readfds)) {
        if (!handleClientMessage(slot)) {
          close(slot.client_fd);
          slot.client_fd = -1;
        }
      }
      if (slot.child_stdout >= 0 && FD_ISSET(slot.child_stdout, &readfds)) {
        drainPipe(slot, slot.child_stdout, "stdout");
      }
      if (slot.child_stderr >= 0 && FD_ISSET(slot.child_stderr, &readfds)) {
        drainPipe(slot, slot.child_stderr, "stderr");
      }
      childExited(slot);
    };
    service(left_);
    service(right_);

    advanceRealtimeIfDue();
    if (left_.display != nullptr && !left_.machine.vbe().graphicsMode()) {
      left_.display->present(left_.machine);
    }
    if (right_.display != nullptr && !right_.machine.vbe().graphicsMode()) {
      right_.display->present(right_.machine);
    }
  }

  if (runtime_status != 0) return runtime_status;
  if (left_.exit_status != 0) return left_.exit_status;
  return right_.exit_status;
}

bool PairRuntimeServer::setup() {
  left_.machine.connectSerialPeer(&right_.machine);
  right_.machine.connectSerialPeer(&left_.machine);
  return setupSlot(left_) && setupSlot(right_) && startChild(left_) && startChild(right_);
}

bool PairRuntimeServer::setupSlot(Slot &slot) {
  if (slot.program.empty()) {
    std::cerr << "machinelab: run-pair missing " << slot.name << " program\n";
    return false;
  }
  if (!slot.script_path.empty()) {
    std::string error;
    if (!slot.script.load(slot.script_path, error)) {
      std::cerr << "machinelab: " << slot.name << ": " << error << "\n";
      return false;
    }
  }
  return setupSharedMemory(slot) && setupDisplay(slot);
}

bool PairRuntimeServer::setupDisplay(Slot &slot) {
  if (options_.display == "headless") {
    slot.display.reset(new HeadlessDisplay());
  } else if (options_.display == "sdl") {
#if defined(MACHINE_LAB_WITH_SDL)
    SdlBackendOptions sdl_options;
    sdl_options.fullscreen = options_.fullscreen;
    sdl_options.integer_scale = options_.integer_scale;
    sdl_options.scale = options_.scale;
    sdl_options.guest_input = true;
    sdl_options.title = "LCOM Pair - " + slot.name;
    slot.display = createSdlBackend(sdl_options);
#else
    std::cerr << "machinelab: SDL backend requested but this build was compiled without SDL3\n";
    return false;
#endif
  } else {
    std::cerr << "machinelab: unknown display backend '" << options_.display << "'\n";
    return false;
  }

  if (slot.display != nullptr && !slot.display->start(slot.machine)) {
    std::cerr << "machinelab: " << slot.name << " display backend failed to start\n";
    return false;
  }
  return true;
}

bool PairRuntimeServer::setupSharedMemory(Slot &slot) {
  slot.shm_size = kAudioShmOffset + slot.machine.ac97().bufferSize();
  std::ostringstream name;
  name << "/lcp" << getpid() << (slot.name == "left" ? "l" : "r") << (std::rand() % 1000);
  slot.shm_name = name.str();
  slot.shm_fd = shm_open(slot.shm_name.c_str(), O_CREAT | O_EXCL | O_RDWR, 0600);
  if (slot.shm_fd < 0) {
    perror("shm_open");
    return false;
  }
  if (ftruncate(slot.shm_fd, static_cast<off_t>(slot.shm_size)) != 0) {
    perror("ftruncate");
    return false;
  }
  void *mapped = mmap(nullptr, slot.shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, slot.shm_fd, 0);
  if (mapped == MAP_FAILED) {
    perror("mmap");
    return false;
  }
  slot.shm_data = static_cast<uint8_t *>(mapped);
  zeroSharedMemory(slot);
  return true;
}

bool PairRuntimeServer::startChild(Slot &slot) {
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
    setenv("MACHINE_LAB_PAIR_SIDE", slot.name.c_str(), 1);
    setenv("LCOM_PAIR_SIDE", slot.name.c_str(), 1);
    if (options_.headless) {
      setenv("MACHINE_LAB_PAIR_AUTO", "1", 1);
      setenv("LCOM_PAIR_AUTO", "1", 1);
    }

    std::vector<char *> argv;
    argv.reserve(slot.program.size() + 1);
    for (std::string &arg : slot.program) argv.push_back(const_cast<char *>(arg.c_str()));
    argv.push_back(nullptr);
    execvp(argv[0], argv.data());
    perror("execvp");
    _exit(127);
  }

  close(sv[1]);
  close(out_pipe[1]);
  close(err_pipe[1]);
  slot.client_fd = sv[0];
  slot.child_stdout = out_pipe[0];
  slot.child_stderr = err_pipe[0];
  setNonBlocking(slot.child_stdout);
  setNonBlocking(slot.child_stderr);
  slot.child_pid = static_cast<int>(pid);
  slot.child_running = true;
  std::cout << "[" << slot.name << "] Running " << slot.program[0] << "...\n";
  return true;
}

void PairRuntimeServer::cleanupSlot(Slot &slot) {
  slot.display.reset();
  if (slot.client_fd >= 0) close(slot.client_fd);
  if (slot.child_stdout >= 0) close(slot.child_stdout);
  if (slot.child_stderr >= 0) close(slot.child_stderr);
  slot.client_fd = slot.child_stdout = slot.child_stderr = -1;
  if (slot.child_running && slot.child_pid > 0) {
    kill(slot.child_pid, SIGTERM);
    waitpid(slot.child_pid, nullptr, 0);
  }
  slot.child_running = false;
  if (slot.shm_data != nullptr) {
    munmap(slot.shm_data, slot.shm_size);
    slot.shm_data = nullptr;
  }
  if (slot.shm_fd >= 0) {
    close(slot.shm_fd);
    slot.shm_fd = -1;
  }
  if (!slot.shm_name.empty()) {
    shm_unlink(slot.shm_name.c_str());
    slot.shm_name.clear();
  }
}

bool PairRuntimeServer::handleClientMessage(Slot &slot) {
  lcom_msg_header_t hdr{};
  if (!protocol::readAll(slot.client_fd, &hdr, sizeof(hdr))) return false;
  if (hdr.size > LCOM_MAX_PAYLOAD) return false;
  uint8_t payload[LCOM_MAX_PAYLOAD];
  if (hdr.size != 0 && !protocol::readAll(slot.client_fd, payload, hdr.size)) return false;

  switch (hdr.type) {
  case LCOM_MSG_HELLO: {
    lcom_hello_t request{};
    if (!protocol::decodePayload(payload, hdr.size, request)) return false;
    lcom_hello_reply_t reply{};
    reply.status = request.version == LCOM_PROTOCOL_VERSION ? 0 : -1;
    reply.version = LCOM_PROTOCOL_VERSION;
    protocol::sendMessage(slot.client_fd, LCOM_MSG_HELLO_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_EXIT:
    return false;
  case LCOM_MSG_CONSOLE_WRITE:
    std::cout << "[" << slot.name << "] ";
    std::cout.write(reinterpret_cast<const char *>(payload), hdr.size);
    std::cout.flush();
    if (slot.display != nullptr) {
      std::ostringstream prefix;
      prefix << "[" << slot.name << "] ";
      std::string p = prefix.str();
      slot.display->consoleWrite(p.data(), p.size());
      slot.display->consoleWrite(reinterpret_cast<const char *>(payload), hdr.size);
    }
    return true;
  case LCOM_MSG_PORT_READ8: {
    if (hdr.size != sizeof(lcom_port_read8_t)) return false;
    lcom_port_read8_t req{};
    if (!protocol::decodePayload(payload, hdr.size, req)) return false;
    uint8_t value = 0;
    bool ok = slot.machine.readPort8(req.port, value);
    lcom_port_read8_reply_t reply{};
    reply.status = ok ? 0 : -1;
    reply.value = value;
    protocol::sendMessage(slot.client_fd, LCOM_MSG_PORT_READ8_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_PORT_WRITE8: {
    if (hdr.size != sizeof(lcom_port_write8_t)) return false;
    lcom_port_write8_t req{};
    if (!protocol::decodePayload(payload, hdr.size, req)) return false;
    sendStatus(slot, hdr.request_id, slot.machine.writePort8(req.port, req.value) ? 0 : -1);
    maybeSatisfyEventWait(left_);
    maybeSatisfyEventWait(right_);
    return true;
  }
  case LCOM_MSG_IRQ_SUBSCRIBE: {
    if (hdr.size != sizeof(lcom_irq_subscribe_t)) return false;
    lcom_irq_subscribe_t req{};
    if (!protocol::decodePayload(payload, hdr.size, req)) return false;
    IrqSubscription sub;
    bool ok = slot.machine.subscribeIrq(req.irq, sub);
    lcom_irq_subscribe_reply_t reply{};
    reply.status = ok ? 0 : -1;
    reply.irq = sub.irq;
    reply.bit_no = sub.bit_no;
    reply.mask = sub.mask;
    protocol::sendMessage(slot.client_fd, LCOM_MSG_IRQ_SUBSCRIBE_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_IRQ_UNSUBSCRIBE: {
    if (hdr.size != sizeof(lcom_irq_unsubscribe_t)) return false;
    lcom_irq_unsubscribe_t req{};
    if (!protocol::decodePayload(payload, hdr.size, req)) return false;
    sendStatus(slot, hdr.request_id, slot.machine.unsubscribeIrq(req.irq) ? 0 : -1);
    return true;
  }
  case LCOM_MSG_EVENT_WAIT:
    slot.waiting_event = true;
    slot.waiting_request_id = hdr.request_id;
    maybeSatisfyEventWait(slot);
    return true;
  case LCOM_MSG_PHYS_MAP: {
    if (hdr.size != sizeof(lcom_phys_map_t)) return false;
    lcom_phys_map_t req{};
    if (!protocol::decodePayload(payload, hdr.size, req)) return false;
    lcom_phys_map_reply_t reply{};
    if (slot.machine.vbe().ownsRange(req.phys, req.length) &&
        req.length <= kFramebufferShmBytes) {
      reply.status = 0;
      std::snprintf(reply.shm_name, sizeof(reply.shm_name), "%s", slot.shm_name.c_str());
      reply.offset = req.phys - slot.machine.vbe().framebufferPhys();
      reply.length = req.length;
    } else if (slot.machine.ac97().ownsRange(req.phys, req.length) &&
               req.length <= slot.shm_size - kAudioShmOffset) {
      reply.status = 0;
      std::snprintf(reply.shm_name, sizeof(reply.shm_name), "%s", slot.shm_name.c_str());
      reply.offset = kAudioShmOffset + (req.phys - slot.machine.ac97().bufferPhys());
      reply.length = req.length;
    } else {
      reply.status = -1;
    }
    protocol::sendMessage(slot.client_fd, LCOM_MSG_PHYS_MAP_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_VBE_GET_MODE_INFO: {
    if (hdr.size != sizeof(lcom_vbe_mode_request_t)) return false;
    lcom_vbe_mode_request_t req{};
    if (!protocol::decodePayload(payload, hdr.size, req)) return false;
    VbeModeInfo info;
    bool ok = slot.machine.vbe().modeInfo(req.mode, info);
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
    protocol::sendMessage(slot.client_fd, LCOM_MSG_VBE_MODE_INFO_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  case LCOM_MSG_VBE_SET_MODE: {
    if (hdr.size != sizeof(lcom_vbe_mode_request_t)) return false;
    lcom_vbe_mode_request_t req{};
    if (!protocol::decodePayload(payload, hdr.size, req)) return false;
    bool ok = slot.machine.vbe().setMode(req.mode);
    zeroSharedMemory(slot);
    sendStatus(slot, hdr.request_id, ok ? 0 : -1);
    return true;
  }
  case LCOM_MSG_VBE_PRESENT:
    syncFramebufferFromSharedMemory(slot);
    if (slot.display != nullptr) slot.display->present(slot.machine);
    sendStatus(slot, hdr.request_id, 0);
    return true;
  case LCOM_MSG_AC97_GET_BUFFER: {
    lcom_ac97_buffer_wire_t reply{};
    reply.status = 0;
    reply.pcm_phys = slot.machine.ac97().bufferPhys();
    reply.pcm_bytes = slot.machine.ac97().bufferSize();
    reply.sample_rate = slot.machine.ac97().sampleRate();
    reply.channels = slot.machine.ac97().channels();
    reply.bits_per_sample = 16;
    protocol::sendMessage(slot.client_fd, LCOM_MSG_AC97_BUFFER_REPLY, hdr.request_id, &reply, sizeof(reply));
    return true;
  }
  default:
    return false;
  }
}

void PairRuntimeServer::sendStatus(Slot &slot, uint32_t request_id, int32_t status) {
  lcom_status_reply_t reply{};
  reply.status = status;
  protocol::sendMessage(slot.client_fd, LCOM_MSG_STATUS, request_id, &reply, sizeof(reply));
}

void PairRuntimeServer::sendEventReply(Slot &slot, uint32_t request_id, uint32_t irq_mask) {
  lcom_event_reply_t reply{};
  reply.status = 0;
  reply.irq_mask = irq_mask;
  reply.tick = slot.machine.tick();
  protocol::sendMessage(slot.client_fd, LCOM_MSG_EVENT_REPLY, request_id, &reply, sizeof(reply));
}

void PairRuntimeServer::maybeSatisfyEventWait(Slot &slot) {
  if (!slot.waiting_event || slot.client_fd < 0) return;
  slot.machine.refreshDeviceIrqs();
  uint32_t pending = slot.machine.pendingIrqs();
  if (pending == 0) return;
  uint32_t mask = slot.machine.consumePendingIrqs();
  slot.waiting_event = false;
  sendEventReply(slot, slot.waiting_request_id, mask);
}

void PairRuntimeServer::advanceAllOnce() {
  if (left_.machine.tick() >= options_.max_ticks || right_.machine.tick() >= options_.max_ticks) {
    std::cerr << "machinelab: run-pair max virtual ticks reached (" << options_.max_ticks << ")\n";
    left_.exit_status = 1;
    right_.exit_status = 1;
    cleanupSlot(left_);
    cleanupSlot(right_);
    return;
  }
  left_.machine.advanceTick();
  right_.machine.advanceTick();
  applyScriptEvents(left_, left_.script.takeDue(left_.machine.tick()));
  applyScriptEvents(right_, right_.script.takeDue(right_.machine.tick()));
}

void PairRuntimeServer::applyScriptEvents(Slot &slot, const std::vector<ScriptEvent> &events) {
  for (const ScriptEvent &ev : events) {
    switch (ev.kind) {
    case ScriptEvent::Kind::Key:
      slot.machine.injectKey(ev.key, ev.pressed);
      break;
    case ScriptEvent::Kind::Mouse:
      slot.machine.injectMouse(ev.dx, ev.dy, ev.buttons);
      break;
    case ScriptEvent::Kind::Text:
      injectText(slot.machine, ev.text);
      break;
    case ScriptEvent::Kind::Rtc:
      slot.machine.rtc().setIsoTime(ev.text);
      break;
    case ScriptEvent::Kind::Tick:
    case ScriptEvent::Kind::Caption:
    case ScriptEvent::Kind::Capture:
      break;
    }
  }
}

void PairRuntimeServer::syncFramebufferFromSharedMemory(Slot &slot) {
  Vbe &vbe = slot.machine.vbe();
  size_t n = std::min(vbe.framebuffer().size(), kFramebufferShmBytes);
  if (slot.shm_data != nullptr && n != 0) {
    std::copy(slot.shm_data, slot.shm_data + n, vbe.framebuffer().begin());
  }
}

void PairRuntimeServer::zeroSharedMemory(Slot &slot) {
  if (slot.shm_data != nullptr && slot.shm_size != 0) {
    std::memset(slot.shm_data, 0, slot.shm_size);
  }
}

void PairRuntimeServer::drainPipe(Slot &slot, int fd, const char *label) {
  char buf[1024];
  for (;;) {
    ssize_t n = read(fd, buf, sizeof(buf));
    if (n > 0) {
      std::ostream &out = std::strcmp(label, "stderr") == 0 ? std::cerr : std::cout;
      out << "[" << slot.name << " " << label << "] ";
      out.write(buf, n);
      out.flush();
      if (slot.display != nullptr) {
        std::ostringstream prefix;
        prefix << "[" << slot.name << " " << label << "] ";
        std::string p = prefix.str();
        slot.display->consoleWrite(p.data(), p.size());
        slot.display->consoleWrite(buf, static_cast<size_t>(n));
      }
      continue;
    }
    if (n == 0) {
      close(fd);
      if (fd == slot.child_stdout) slot.child_stdout = -1;
      if (fd == slot.child_stderr) slot.child_stderr = -1;
      return;
    }
    if (errno == EAGAIN || errno == EWOULDBLOCK || errno == EINTR) return;
    return;
  }
}

bool PairRuntimeServer::childExited(Slot &slot) {
  if (!slot.child_running || slot.child_pid <= 0) return true;
  int status = 0;
  pid_t ret = waitpid(slot.child_pid, &status, WNOHANG);
  if (ret == 0) return false;
  if (ret == slot.child_pid) {
    slot.child_running = false;
    slot.child_pid = -1;
    if (WIFEXITED(status)) {
      slot.exit_status = WEXITSTATUS(status);
      std::cout << "[" << slot.name << "] Program exited with status "
                << WEXITSTATUS(status) << "\n";
    } else if (WIFSIGNALED(status)) {
      slot.exit_status = 128 + WTERMSIG(status);
      std::cout << "[" << slot.name << "] Program terminated by signal "
                << WTERMSIG(status) << "\n";
    } else {
      slot.exit_status = 1;
    }
    if (slot.client_fd >= 0) {
      close(slot.client_fd);
      slot.client_fd = -1;
    }
    return true;
  }
  return false;
}

bool PairRuntimeServer::anyAlive() const {
  return left_.child_running || right_.child_running ||
         left_.client_fd >= 0 || right_.client_fd >= 0;
}

bool PairRuntimeServer::shouldAdvanceHeadless() const {
  const auto ready = [](const Slot &slot) {
    return !slot.child_running || slot.waiting_event;
  };
  return (left_.child_running || right_.child_running) &&
         ready(left_) && ready(right_);
}

} // namespace lcom
