#ifndef LCOM_NG_RUNTIME_SERVER_HPP
#define LCOM_NG_RUNTIME_SERVER_HPP

#include "Machine.hpp"
#include "Script.hpp"
#include "Trace.hpp"
#include "../backends/AudioBackend.hpp"
#include "../backends/DisplayBackend.hpp"

#include <cstdint>
#include <memory>
#include <string>
#include <vector>

namespace lcom {

struct RuntimeOptions {
  bool headless = true;
  bool realtime = false;
  bool no_realtime = false;
  std::string display = "headless";
  std::string audio = "null";
  std::string audio_wav_path;
  bool fullscreen = false;
  bool integer_scale = false;
  int scale = 1;
  uint64_t max_ticks = 100000;
  std::string script_path;
  std::string trace_path;
  std::string dump_frame_path;
  std::string frame_dir_path;
  std::string video_path;
  uint32_t video_fps = 60;
  std::string rtc_time;
  std::vector<std::string> program;
};

class RuntimeServer {
public:
  explicit RuntimeServer(RuntimeOptions options);
  ~RuntimeServer();

  int run();

private:
  bool setup();
  bool setupSharedMemory();
  bool startChild();
  void cleanupChild();
  void cleanupSharedMemory();

  bool handleClientMessage();
  void handleConsoleWrite(const char *data, size_t size);
  void sendStatus(uint32_t request_id, int32_t status);
  void sendEventReply(uint32_t request_id, uint32_t irq_mask);
  void maybeSatisfyEventWait();
  void advanceVirtualTimeOnce();
  void syncFramebufferFromSharedMemory();
  void syncAudioFromSharedMemory();
  void zeroSharedFramebuffer();
  bool setupVideoCapture();
  void dumpVideoFrame();
  bool renderVideo();
  void cleanupVideoCapture();
  bool setupDisplay();
  bool setupAudio();
  bool updateAudioBackendFromDevice(bool was_playing, bool force_play);
  void drainPipe(int fd, const char *label);
  bool childExited();

  RuntimeOptions options_;
  Machine machine_;
  Script script_;
  Trace trace_;
  std::unique_ptr<DisplayBackend> display_;
  std::unique_ptr<AudioBackend> audio_;
  bool audio_backend_playing_ = false;

  int client_fd_ = -1;
  int child_stdout_ = -1;
  int child_stderr_ = -1;
  int child_pid_ = -1;
  bool child_running_ = false;

  bool waiting_event_ = false;
  uint32_t waiting_request_id_ = 0;

  std::string shm_name_;
  int shm_fd_ = -1;
  uint8_t *shm_data_ = nullptr;
  size_t shm_size_ = 0;

  std::string active_frame_dir_;
  bool remove_frame_dir_ = false;
  uint32_t frame_index_ = 0;
};

} // namespace lcom

#endif
