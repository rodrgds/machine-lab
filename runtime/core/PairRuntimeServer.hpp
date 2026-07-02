#ifndef MACHINE_LAB_PAIR_RUNTIME_SERVER_HPP
#define MACHINE_LAB_PAIR_RUNTIME_SERVER_HPP

#include "Machine.hpp"
#include "Script.hpp"
#include "../backends/DisplayBackend.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace lcom {

struct PairRuntimeOptions {
  std::vector<std::string> left_program;
  std::vector<std::string> right_program;
  std::string left_script_path;
  std::string right_script_path;
  uint64_t max_ticks = 100000;
  bool headless = false;
  bool realtime = true;
  bool no_realtime = false;
  bool fullscreen = false;
  bool integer_scale = false;
  int scale = 1;
  std::string display = "sdl";
};

class PairRuntimeServer {
public:
  explicit PairRuntimeServer(PairRuntimeOptions options);
  ~PairRuntimeServer();

  int run();

private:
  struct Slot {
    std::string name;
    std::vector<std::string> program;
    std::string script_path;
    Machine machine;
    Script script;
    std::unique_ptr<DisplayBackend> display;
    int client_fd = -1;
    int child_stdout = -1;
    int child_stderr = -1;
    int child_pid = -1;
    bool child_running = false;
    bool waiting_event = false;
    uint32_t waiting_request_id = 0;
    std::string shm_name;
    int shm_fd = -1;
    uint8_t *shm_data = nullptr;
    size_t shm_size = 0;
  };

  bool setup();
  bool setupSlot(Slot &slot);
  bool setupDisplay(Slot &slot);
  bool setupSharedMemory(Slot &slot);
  bool startChild(Slot &slot);
  void cleanupSlot(Slot &slot);
  bool handleClientMessage(Slot &slot);
  void sendStatus(Slot &slot, uint32_t request_id, int32_t status);
  void sendEventReply(Slot &slot, uint32_t request_id, uint32_t irq_mask);
  void maybeSatisfyEventWait(Slot &slot);
  void advanceAllOnce();
  void applyScriptEvents(Slot &slot, const std::vector<ScriptEvent> &events);
  void syncFramebufferFromSharedMemory(Slot &slot);
  void zeroSharedMemory(Slot &slot);
  void drainPipe(Slot &slot, int fd, const char *label);
  bool childExited(Slot &slot);
  bool anyAlive() const;
  bool shouldAdvanceHeadless() const;

  PairRuntimeOptions options_;
  Slot left_;
  Slot right_;
};

} // namespace lcom

#endif
