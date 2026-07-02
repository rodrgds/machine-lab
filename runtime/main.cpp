#include "cli/LabCatalog.hpp"
#include "cli/FileOps.hpp"
#include "core/RuntimeServer.hpp"
#include "core/PairRuntimeServer.hpp"

#include <CLI/CLI.hpp>

#include <algorithm>
#include <cctype>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <sstream>
#include <string>
#include <vector>

using lcom::RuntimeOptions;
using lcom::RuntimeServer;
using lcom::PairRuntimeOptions;
using lcom::PairRuntimeServer;
using lcom::cli::StudentLabSpec;
using lcom::cli::copyIfExists;
using lcom::cli::copyTreeIfExists;
using lcom::cli::makeExecutable;
using lcom::cli::shellQuote;
using lcom::cli::studentLabSpec;
using lcom::cli::studentLabSpecs;
using lcom::cli::writeTextFile;

static void configureCliApp(CLI::App &app) {
  app.description("Machine Lab userspace machine runtime");
  app.require_subcommand(0, 1);

  auto *run = app.add_subcommand("run", "Run a student program inside the Machine Lab runtime");
  run->add_option("program", "Program and arguments after runtime options");
  run->add_flag("--headless", "Use deterministic headless display");
  run->add_option("--display", "Display backend: headless or sdl");
  run->add_option("--audio", "Audio backend: null, sdl, or wav:<path>");
  run->add_option("--audio-wav", "Shortcut for --audio wav:<path>");
  run->add_flag("--realtime", "Advance virtual time from host time");
  run->add_flag("--no-realtime", "Disable automatic realtime for SDL");
  run->add_flag("--fullscreen", "Fullscreen SDL window");
  run->add_option("--scale", "Initial SDL window scale");
  run->add_flag("--integer-scale", "Use integer SDL scaling");
  run->add_option("--script", "Inject scripted keyboard/mouse/RTC events");
  run->add_option("--trace", "Write JSONL device trace");
  run->add_option("--dump-frame", "Dump current framebuffer as PPM");
  run->add_option("--frame-dir", "Dump every presented frame");
  run->add_option("--video", "Render presented frames to MP4 with ffmpeg");
  run->add_option("--video-fps", "FPS for --video output");
  run->add_option("--rtc", "Freeze RTC, e.g. 2026-06-16T12:00:00");
  run->add_option("--max-ticks", "Guard against hung headless programs");

  auto *run_pair = app.add_subcommand("run-pair", "Run two programs with bridged virtual serial ports");
  run_pair->add_option("left-program", "Left program and arguments");
  run_pair->add_option("right-program", "Right program and arguments after --right");
  run_pair->add_flag("--headless", "Use deterministic headless displays and auto pair smoke mode");
  run_pair->add_option("--display", "Display backend: headless or sdl");
  run_pair->add_flag("--realtime", "Advance virtual time from host time");
  run_pair->add_flag("--no-realtime", "Disable realtime ticking");
  run_pair->add_flag("--fullscreen", "Fullscreen SDL windows");
  run_pair->add_option("--scale", "Initial SDL window scale");
  run_pair->add_flag("--integer-scale", "Use integer SDL scaling");
  run_pair->add_option("--left-script", "Inject script into the left program");
  run_pair->add_option("--right-script", "Inject script into the right program");
  run_pair->add_option("--max-ticks", "Guard against hung paired programs");
  run_pair->add_flag("--right", "Separate the right program from the left program");

  auto *replay = app.add_subcommand("replay", "Replay a script against a student program");
  replay->add_option("script", "Script file")->required();
  replay->add_option("program", "Program and arguments after runtime options");
  replay->add_flag("--headless", "Use deterministic headless display");
  replay->add_option("--display", "Display backend: headless or sdl");
  replay->add_option("--audio", "Audio backend: null, sdl, or wav:<path>");
  replay->add_option("--audio-wav", "Shortcut for --audio wav:<path>");
  replay->add_flag("--realtime", "Advance virtual time from host time");
  replay->add_flag("--no-realtime", "Disable automatic realtime for SDL");
  replay->add_flag("--fullscreen", "Fullscreen SDL window");
  replay->add_option("--scale", "Initial SDL window scale");
  replay->add_flag("--integer-scale", "Use integer SDL scaling");
  replay->add_option("--trace", "Write JSONL device trace");
  replay->add_option("--dump-frame", "Dump current framebuffer as PPM");
  replay->add_option("--frame-dir", "Dump every presented frame");
  replay->add_option("--video", "Render presented frames to MP4 with ffmpeg");
  replay->add_option("--video-fps", "FPS for --video output");
  replay->add_option("--rtc", "Freeze RTC, e.g. 2026-06-16T12:00:00");
  replay->add_option("--max-ticks", "Guard against hung headless programs");

  auto *setup = app.add_subcommand("setup", "Create a student lab workspace");
  setup->add_option("project-dir", "Project directory to create or update (default: .)");
  setup->add_flag("--force", "Overwrite generated starter files");

  auto *test = app.add_subcommand("test", "Build and run predefined tests for a student lab");
  test->add_option("lab", "rtc, timer, kbd, mouse, graphics, audio, uart, or lab1-lab7")->required();
  test->add_option("--project", "Student project directory (default: .)");
  test->add_flag("--keep-build", "Keep the temporary test build directory");

  auto *bundle = app.add_subcommand("bundle", "Create a runnable Machine Lab app bundle");
  bundle->add_option("project-dir", "Project directory")->required();
  bundle->add_option("--program", "Student binary to bundle")->required();
  bundle->add_option("--name", "Bundle/app name");
  bundle->add_option("--output", "Output bundle executable or directory");
  bundle->add_option("--format", "Bundle format: single or dir");
  bundle->add_option("--script", "Script to include and run with the app");
  bundle->add_option("--display", "Display backend in generated launcher");
  bundle->add_option("--audio", "Audio backend in generated launcher");
  bundle->add_flag("--headless", "Generate a headless launcher");
  bundle->add_flag("--realtime", "Generate a realtime launcher");
  bundle->add_flag("--no-realtime", "Generate a non-realtime launcher");
  bundle->add_flag("--dir", "Shortcut for --format dir");
  bundle->add_flag("--single-file", "Shortcut for --format single");

  auto *lab = app.add_subcommand("lab", "Inspect lab function requests");
  lab->add_subcommand("list", "List labs");
  auto *show = lab->add_subcommand("show", "Show requested functions for a lab");
  show->add_option("lab", "rtc, timer, kbd, mouse, graphics, audio, uart, or lab1-lab7")->required();

  auto *docs = app.add_subcommand("docs", "Generate command documentation");
  docs->add_subcommand("cli", "Print CLI command reference");

  auto *completion = app.add_subcommand("completion", "Print shell completion script");
  completion->add_option("shell", "bash, zsh, or fish")->required();
}

static void usage() {
  CLI::App app;
  configureCliApp(app);
  std::cout << app.help();
}

static void subcommandUsage(const std::string &name) {
  CLI::App app;
  configureCliApp(app);
  try {
    std::cout << app.get_subcommand(name)->help();
  } catch (const CLI::OptionNotFound &) {
    std::cout << app.help();
  }
}

struct BundleOptions {
  std::filesystem::path project_dir;
  std::filesystem::path program;
  std::filesystem::path output_path;
  std::filesystem::path script;
  std::string name;
  std::string display = "sdl";
  std::string audio = "sdl";
  std::string format = "single";
  bool realtime = true;
};

struct SetupOptions {
  std::filesystem::path project_dir = ".";
  bool force = false;
};

struct TestOptions {
  std::string lab;
  std::filesystem::path project_dir = ".";
  bool keep_build = false;
};

static std::string jsonEscape(const std::string &s) {
  std::string out;
  for (char c : s) {
    if (c == '"' || c == '\\') {
      out.push_back('\\');
      out.push_back(c);
    } else if (c == '\n') {
      out += "\\n";
    } else {
      out.push_back(c);
    }
  }
  return out;
}

static int labList() {
  for (const StudentLabSpec &spec : studentLabSpecs()) {
    std::cout << spec.id << " (" << spec.dir << ")  " << spec.title << "\n";
  }
  return 0;
}

static int labShow(const std::string &lab) {
  const StudentLabSpec *spec = studentLabSpec(lab);
  if (spec == nullptr) {
    std::cerr << "machinelab: unknown lab " << lab << "\n";
    return 1;
  }
  std::cout << spec->id << " (" << spec->dir << "): " << spec->title << "\n";
  std::cout << "sources:";
  for (const std::string &source : spec->sources) std::cout << " labs/" << spec->dir << "/" << source;
  std::cout << "\nfunctions:\n";
  for (const std::string &fn : spec->functions) std::cout << "  " << fn << "\n";
  return 0;
}

static int cliDocs() {
  CLI::App app;
  configureCliApp(app);
  std::cout << "# machinelab CLI\n\n";
  std::cout << "```text\n" << app.help() << "```\n";
  std::cout << "\nCommon examples:\n\n";
  std::cout << "```sh\n";
  std::cout << "machinelab run build/examples/flappy_bird\n";
  std::cout << "machinelab run-pair --headless build/examples/uart_peer_sender --right build/examples/uart_peer_receiver\n";
  std::cout << "machinelab replay scripts/flappy_demo.mlabscript --headless --video build/flappy.mp4 -- build/examples/flappy_bird\n";
  std::cout << "machinelab replay scripts/ninjix_level1_demo.mlabscript --headless --video build/ninjix-demo.mp4 --video-fps 12 --max-ticks 3600 -- build/examples/ninjix\n";
  std::cout << "machinelab setup student\n";
  std::cout << "machinelab test rtc --project student\n";
  std::cout << "machinelab bundle . --program build/examples/flappy_bird --name flappy-bird\n";
  std::cout << "machinelab completion zsh > _machinelab\n";
  std::cout << "```\n";
  return 0;
}

static int completionScript(const std::string &shell) {
  const char *commands = "run run-pair replay setup test bundle lab docs completion help";
  const char *run_opts =
      "--headless --display --audio --audio-wav --realtime --no-realtime --fullscreen "
      "--scale --integer-scale --script --trace --dump-frame --frame-dir --video --video-fps "
      "--rtc --max-ticks";
  const char *replay_opts =
      "--headless --display --audio --audio-wav --realtime --no-realtime --fullscreen "
      "--scale --integer-scale --trace --dump-frame --frame-dir --video --video-fps "
      "--rtc --max-ticks";
  const char *bundle_opts =
      "--program --name --output --format --script --display --audio --headless --realtime "
      "--no-realtime --dir --single-file";
  const char *setup_opts = "--force";
  const char *test_opts = "--project --keep-build";
  const char *lab_names = "rtc timer kbd mouse graphics audio uart lab1 lab2 lab3 lab4 lab5 lab6 lab7";

  if (shell == "bash") {
    std::cout
        << "_machinelab_complete() {\n"
        << "  local cur prev cmd\n"
        << "  COMPREPLY=()\n"
        << "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
        << "  cmd=\"${COMP_WORDS[1]}\"\n"
        << "  if [ $COMP_CWORD -eq 1 ]; then COMPREPLY=( $(compgen -W \"" << commands << "\" -- \"$cur\") ); return 0; fi\n"
        << "  case \"$cmd\" in\n"
        << "    run) COMPREPLY=( $(compgen -W \"" << run_opts << "\" -- \"$cur\") ) ;;\n"
        << "    replay) COMPREPLY=( $(compgen -W \"" << replay_opts << "\" -- \"$cur\") ) ;;\n"
        << "    run-pair) COMPREPLY=( $(compgen -W \"--headless --display --realtime --no-realtime --fullscreen --scale --integer-scale --left-script --right-script --max-ticks --right\" -- \"$cur\") ) ;;\n"
        << "    setup) COMPREPLY=( $(compgen -W \"" << setup_opts << "\" -- \"$cur\") ) ;;\n"
        << "    test) COMPREPLY=( $(compgen -W \"" << lab_names << " " << test_opts << "\" -- \"$cur\") ) ;;\n"
        << "    bundle) COMPREPLY=( $(compgen -W \"" << bundle_opts << "\" -- \"$cur\") ) ;;\n"
        << "    lab) COMPREPLY=( $(compgen -W \"list show " << lab_names << "\" -- \"$cur\") ) ;;\n"
        << "    completion) COMPREPLY=( $(compgen -W \"bash zsh fish\" -- \"$cur\") ) ;;\n"
        << "  esac\n"
        << "}\ncomplete -F _machinelab_complete machinelab\n";
    return 0;
  }
  if (shell == "zsh") {
    std::cout
        << "#compdef machinelab\n"
        << "_machinelab() {\n"
        << "  local -a commands\n"
        << "  commands=(run run-pair replay setup test bundle lab docs completion help)\n"
        << "  if (( CURRENT == 2 )); then _describe 'command' commands; return; fi\n"
        << "  case $words[2] in\n"
        << "    run) _arguments '--headless' '--display[backend]:' '--audio[backend]:' '--audio-wav[file]:file:_files' '--realtime' '--no-realtime' '--fullscreen' '--scale[n]:' '--integer-scale' '--script[file]:file:_files' '--trace[file]:file:_files' '--dump-frame[file]:file:_files' '--frame-dir[dir]:dir:_files -/' '--video[file]:file:_files' '--video-fps[n]:' '--rtc[iso-time]:' '--max-ticks[n]:' '*::program:_files' ;;\n"
        << "    replay) _arguments '--headless' '--display[backend]:' '--audio[backend]:' '--audio-wav[file]:file:_files' '--realtime' '--no-realtime' '--fullscreen' '--scale[n]:' '--integer-scale' '--trace[file]:file:_files' '--dump-frame[file]:file:_files' '--frame-dir[dir]:dir:_files -/' '--video[file]:file:_files' '--video-fps[n]:' '--rtc[iso-time]:' '--max-ticks[n]:' '1:script:_files' '*::program:_files' ;;\n"
        << "    run-pair) _arguments '--headless' '--display[backend]:' '--realtime' '--no-realtime' '--fullscreen' '--scale[n]:' '--integer-scale' '--left-script[file]:file:_files' '--right-script[file]:file:_files' '--max-ticks[n]:' '--right' '*::program:_files' ;;\n"
        << "    setup) _arguments '--force' '1:project-dir:_files -/' ;;\n"
        << "    test) _arguments '--project[dir]:dir:_files -/' '--keep-build' '1:lab:(rtc timer kbd mouse graphics audio uart lab1 lab2 lab3 lab4 lab5 lab6 lab7)' ;;\n"
        << "    bundle) _arguments '--program[student binary]:file:_files' '--name[name]:' '--output[path]:file:_files' '--format[format]:(single dir)' '--script[script]:file:_files' '--display[backend]:' '--audio[backend]:' '--headless' '--realtime' '--no-realtime' '--dir' '--single-file' ;;\n"
        << "    lab) _arguments '1:action:(list show)' '2:lab:(rtc timer kbd mouse graphics audio uart lab1 lab2 lab3 lab4 lab5 lab6 lab7)' ;;\n"
        << "    completion) _arguments '1:shell:(bash zsh fish)' ;;\n"
        << "  esac\n"
        << "}\n_machinelab \"$@\"\n";
    return 0;
  }
  if (shell == "fish") {
    std::cout
        << "complete -c machinelab -f -n '__fish_use_subcommand' -a '" << commands << "'\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run replay' -l display -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run replay' -l audio -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run replay' -l realtime\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run replay' -l no-realtime\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run' -l script -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run replay' -l video -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l left-script -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l right-script -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l max-ticks -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l headless\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l display -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l realtime\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l no-realtime\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from run-pair' -l right\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from setup' -l force\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from test' -l project -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from test' -l keep-build\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from test' -a 'rtc timer kbd mouse graphics audio uart lab1 lab2 lab3 lab4 lab5 lab6 lab7'\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from lab' -a 'list show rtc timer kbd mouse graphics audio uart lab1 lab2 lab3 lab4 lab5 lab6 lab7'\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from bundle' -l program -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from bundle' -l output -r\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from bundle' -l format -a 'single dir'\n"
        << "complete -c machinelab -n '__fish_seen_subcommand_from completion' -a 'bash zsh fish'\n";
    return 0;
  }
  std::cerr << "machinelab: unknown completion shell " << shell << "\n";
  return 1;
}

static bool parseRun(int argc, char **argv, int start, RuntimeOptions &opts) {
  for (int i = start; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--") {
      for (int j = i + 1; j < argc; j++) opts.program.emplace_back(argv[j]);
      return !opts.program.empty();
    }
    if (arg == "--headless") {
      opts.headless = true;
      opts.display = "headless";
      if (opts.audio_wav_path.empty()) opts.audio = "null";
    } else if (arg == "--display" && i + 1 < argc) {
      opts.display = argv[++i];
      opts.headless = opts.display == "headless";
      if (opts.headless && opts.audio_wav_path.empty()) opts.audio = "null";
      if (opts.display == "sdl" && opts.audio == "null" && opts.audio_wav_path.empty()) {
        opts.audio = "sdl";
      }
    } else if (arg == "--audio" && i + 1 < argc) {
      opts.audio = argv[++i];
    } else if (arg == "--audio-wav" && i + 1 < argc) {
      opts.audio_wav_path = argv[++i];
    } else if (arg == "--realtime") {
      opts.realtime = true;
      opts.no_realtime = false;
    } else if (arg == "--no-realtime") {
      opts.realtime = false;
      opts.no_realtime = true;
    } else if (arg == "--fullscreen") {
      opts.fullscreen = true;
    } else if (arg == "--integer-scale") {
      opts.integer_scale = true;
    } else if (arg == "--scale" && i + 1 < argc) {
      opts.scale = std::atoi(argv[++i]);
    } else if (arg == "--script" && i + 1 < argc) {
      opts.script_path = argv[++i];
    } else if (arg == "--trace" && i + 1 < argc) {
      opts.trace_path = argv[++i];
    } else if (arg == "--dump-frame" && i + 1 < argc) {
      opts.dump_frame_path = argv[++i];
    } else if (arg == "--frame-dir" && i + 1 < argc) {
      opts.frame_dir_path = argv[++i];
    } else if (arg == "--video" && i + 1 < argc) {
      opts.video_path = argv[++i];
    } else if (arg == "--video-fps" && i + 1 < argc) {
      opts.video_fps = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
    } else if (arg == "--rtc" && i + 1 < argc) {
      opts.rtc_time = argv[++i];
    } else if (arg == "--max-ticks" && i + 1 < argc) {
      opts.max_ticks = std::strtoull(argv[++i], nullptr, 10);
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "machinelab: unknown option " << arg << "\n";
      return false;
    } else {
      for (int j = i; j < argc; j++) opts.program.emplace_back(argv[j]);
      return !opts.program.empty();
    }
  }
  return false;
}

static bool parseReplay(int argc, char **argv, int start, RuntimeOptions &opts) {
  bool have_script = false;
  for (int i = start; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--") {
      if (!have_script) return false;
      for (int j = i + 1; j < argc; j++) opts.program.emplace_back(argv[j]);
      return !opts.program.empty();
    }
    if (arg == "--headless") {
      opts.headless = true;
      opts.display = "headless";
      if (opts.audio_wav_path.empty()) opts.audio = "null";
    } else if (arg == "--display" && i + 1 < argc) {
      opts.display = argv[++i];
      opts.headless = opts.display == "headless";
      if (opts.headless && opts.audio_wav_path.empty()) opts.audio = "null";
      if (opts.display == "sdl" && opts.audio == "null" && opts.audio_wav_path.empty()) {
        opts.audio = "sdl";
      }
    } else if (arg == "--audio" && i + 1 < argc) {
      opts.audio = argv[++i];
    } else if (arg == "--audio-wav" && i + 1 < argc) {
      opts.audio_wav_path = argv[++i];
    } else if (arg == "--realtime") {
      opts.realtime = true;
      opts.no_realtime = false;
    } else if (arg == "--no-realtime") {
      opts.realtime = false;
      opts.no_realtime = true;
    } else if (arg == "--fullscreen") {
      opts.fullscreen = true;
    } else if (arg == "--integer-scale") {
      opts.integer_scale = true;
    } else if (arg == "--scale" && i + 1 < argc) {
      opts.scale = std::atoi(argv[++i]);
    } else if (arg == "--trace" && i + 1 < argc) {
      opts.trace_path = argv[++i];
    } else if (arg == "--dump-frame" && i + 1 < argc) {
      opts.dump_frame_path = argv[++i];
    } else if (arg == "--frame-dir" && i + 1 < argc) {
      opts.frame_dir_path = argv[++i];
    } else if (arg == "--video" && i + 1 < argc) {
      opts.video_path = argv[++i];
    } else if (arg == "--video-fps" && i + 1 < argc) {
      opts.video_fps = static_cast<uint32_t>(std::strtoul(argv[++i], nullptr, 10));
    } else if (arg == "--rtc" && i + 1 < argc) {
      opts.rtc_time = argv[++i];
    } else if (arg == "--max-ticks" && i + 1 < argc) {
      opts.max_ticks = std::strtoull(argv[++i], nullptr, 10);
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "machinelab: unknown replay option " << arg << "\n";
      return false;
    } else if (!have_script) {
      opts.script_path = arg;
      have_script = true;
    } else {
      for (int j = i; j < argc; j++) opts.program.emplace_back(argv[j]);
      return !opts.program.empty();
    }
  }
  return false;
}

static bool parseRunPair(int argc, char **argv, int start, PairRuntimeOptions &opts) {
  bool right = false;
  for (int i = start; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--right" || arg == "--") {
      right = true;
    } else if (arg == "--headless") {
      opts.headless = true;
      opts.display = "headless";
      opts.realtime = false;
      opts.no_realtime = true;
    } else if (arg == "--display" && i + 1 < argc) {
      opts.display = argv[++i];
      opts.headless = opts.display == "headless";
      if (opts.headless) {
        opts.realtime = false;
        opts.no_realtime = true;
      } else if (!opts.no_realtime) {
        opts.realtime = true;
      }
    } else if (arg == "--realtime") {
      opts.realtime = true;
      opts.no_realtime = false;
    } else if (arg == "--no-realtime") {
      opts.realtime = false;
      opts.no_realtime = true;
    } else if (arg == "--fullscreen") {
      opts.fullscreen = true;
    } else if (arg == "--integer-scale") {
      opts.integer_scale = true;
    } else if (arg == "--scale" && i + 1 < argc) {
      opts.scale = std::atoi(argv[++i]);
    } else if (arg == "--left-script" && i + 1 < argc) {
      opts.left_script_path = argv[++i];
    } else if (arg == "--right-script" && i + 1 < argc) {
      opts.right_script_path = argv[++i];
    } else if (arg == "--max-ticks" && i + 1 < argc) {
      opts.max_ticks = std::strtoull(argv[++i], nullptr, 10);
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "machinelab: unknown run-pair option " << arg << "\n";
      return false;
    } else if (!right) {
      opts.left_program.emplace_back(arg);
    } else {
      opts.right_program.emplace_back(arg);
    }
  }
  return !opts.left_program.empty() && !opts.right_program.empty();
}

static PairRuntimeOptions defaultPairRuntimeOptions() {
  PairRuntimeOptions opts;
#if defined(MACHINE_LAB_WITH_SDL)
  opts.headless = false;
  opts.display = "sdl";
  opts.realtime = true;
#else
  opts.headless = true;
  opts.display = "headless";
  opts.realtime = false;
  opts.no_realtime = true;
#endif
  return opts;
}

static void applyRunDefaults(RuntimeOptions &opts) {
  if (!opts.headless && !opts.realtime && !opts.no_realtime) opts.realtime = true;
}

static RuntimeOptions defaultRuntimeOptions() {
  RuntimeOptions opts;
#if defined(MACHINE_LAB_WITH_SDL)
  opts.headless = false;
  opts.display = "sdl";
  opts.audio = "sdl";
#else
  opts.headless = true;
  opts.display = "headless";
  opts.audio = "null";
#endif
  return opts;
}

static bool parseSetup(int argc, char **argv, int start, SetupOptions &opts) {
  bool have_project = false;
  for (int i = start; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--force") {
      opts.force = true;
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "machinelab: unknown setup option " << arg << "\n";
      return false;
    } else if (!have_project) {
      opts.project_dir = arg;
      have_project = true;
    } else {
      std::cerr << "machinelab: setup accepts at most one project directory\n";
      return false;
    }
  }
  return true;
}

static bool parseTest(int argc, char **argv, int start, TestOptions &opts) {
  bool have_lab = false;
  for (int i = start; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--project" && i + 1 < argc) {
      opts.project_dir = argv[++i];
    } else if (arg == "--keep-build") {
      opts.keep_build = true;
    } else if (!arg.empty() && arg[0] == '-') {
      std::cerr << "machinelab: unknown test option " << arg << "\n";
      return false;
    } else if (!have_lab) {
      opts.lab = arg;
      have_lab = true;
    } else {
      std::cerr << "machinelab: test accepts one lab name\n";
      return false;
    }
  }
  return have_lab;
}

static bool parseBundle(int argc, char **argv, int start, BundleOptions &opts) {
  if (start >= argc) {
    std::cerr << "machinelab: bundle requires <project-dir>\n";
    return false;
  }
  opts.project_dir = argv[start++];

  for (int i = start; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--program" && i + 1 < argc) {
      opts.program = argv[++i];
    } else if (arg == "--name" && i + 1 < argc) {
      opts.name = argv[++i];
    } else if (arg == "--output" && i + 1 < argc) {
      opts.output_path = argv[++i];
    } else if (arg == "--format" && i + 1 < argc) {
      opts.format = argv[++i];
    } else if (arg == "--script" && i + 1 < argc) {
      opts.script = argv[++i];
    } else if (arg == "--display" && i + 1 < argc) {
      opts.display = argv[++i];
    } else if (arg == "--audio" && i + 1 < argc) {
      opts.audio = argv[++i];
    } else if (arg == "--headless") {
      opts.display = "headless";
      opts.audio = "null";
      opts.realtime = false;
    } else if (arg == "--realtime") {
      opts.realtime = true;
    } else if (arg == "--no-realtime") {
      opts.realtime = false;
    } else if (arg == "--dir") {
      opts.format = "dir";
    } else if (arg == "--single-file") {
      opts.format = "single";
    } else {
      std::cerr << "machinelab: unknown bundle option " << arg << "\n";
      return false;
    }
  }

  if (opts.program.empty()) {
    std::cerr << "machinelab: bundle requires --program <binary>\n";
    return false;
  }
  if (opts.format != "single" && opts.format != "dir") {
    std::cerr << "machinelab: bundle --format must be 'single' or 'dir'\n";
    return false;
  }
  if (opts.name.empty()) opts.name = opts.program.filename().string();
  if (opts.output_path.empty()) {
    opts.output_path = opts.project_dir / "dist" / (opts.name + ".mlab");
  }
  return true;
}

static bool writeSingleFileBundle(const std::filesystem::path &stage,
                                  const std::filesystem::path &output,
                                  std::string &error) {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::create_directories(output.parent_path(), ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  if (fs::exists(output, ec) && fs::is_directory(output, ec)) {
    error = "output path is a directory";
    return false;
  }

  fs::path archive = output;
  archive += ".payload.tar.gz";
  fs::remove(archive, ec);
  std::string command = "tar -czf " + shellQuote(archive) + " -C " + shellQuote(stage) + " .";
  if (std::system(command.c_str()) != 0) {
    error = "tar failed while creating payload";
    fs::remove(archive, ec);
    return false;
  }

  std::ofstream out(output, std::ios::binary);
  if (!out.is_open()) {
    error = "could not open output file";
    fs::remove(archive, ec);
    return false;
  }
  out << "#!/usr/bin/env sh\n"
      << "set -eu\n"
      << "tmp=${TMPDIR:-/tmp}\n"
      << "dir=$(mktemp -d \"${tmp%/}/machinelab-bundle.XXXXXX\")\n"
      << "cleanup() { rm -rf \"$dir\"; }\n"
      << "trap cleanup EXIT INT TERM\n"
      << "line=$(awk '/^__LCOM_BUNDLE_PAYLOAD__$/ { print NR + 1; exit }' \"$0\")\n"
      << "tail -n +\"$line\" \"$0\" | tar -xzf - -C \"$dir\"\n"
      << "exec \"$dir/run.sh\" \"$@\"\n"
      << "exit 0\n"
      << "__LCOM_BUNDLE_PAYLOAD__\n";

  std::ifstream in(archive, std::ios::binary);
  if (!in.is_open()) {
    error = "could not reopen payload archive";
    fs::remove(archive, ec);
    return false;
  }
  out << in.rdbuf();
  out.close();
  fs::remove(archive, ec);
  makeExecutable(output);
  return true;
}

static int createBundleDirectory(const BundleOptions &opts,
                                 const char *argv0,
                                 const std::filesystem::path &output) {
  namespace fs = std::filesystem;
  std::error_code ec;

  fs::path project = fs::absolute(opts.project_dir);
  fs::path program = fs::absolute(opts.program);
  if (!fs::exists(project, ec) || !fs::is_directory(project, ec)) {
    std::cerr << "machinelab: project directory does not exist: " << project << "\n";
    return 1;
  }
  if (!fs::exists(program, ec) || !fs::is_regular_file(program, ec)) {
    std::cerr << "machinelab: program does not exist: " << program << "\n";
    return 1;
  }

  fs::create_directories(output / "bin", ec);
  fs::create_directories(output / "app", ec);
  fs::create_directories(output / "sdk" / "lib", ec);
  fs::create_directories(output / "scripts", ec);
  if (ec) {
    std::cerr << "machinelab: could not create bundle: " << ec.message() << "\n";
    return 1;
  }

  std::string error;
  fs::path runtime = fs::absolute(argv0);
  fs::path bundled_runtime = output / "bin" / runtime.filename();
  fs::path bundled_program = output / "app" / program.filename();
  if (!copyIfExists(runtime, bundled_runtime, error) ||
      !copyIfExists(program, bundled_program, error)) {
    std::cerr << "machinelab: bundle copy failed: " << error << "\n";
    return 1;
  }
  makeExecutable(bundled_runtime);
  makeExecutable(bundled_program);

  fs::path bundled_script;
  if (!opts.script.empty()) {
    fs::path script = fs::absolute(opts.script);
    bundled_script = output / "scripts" / script.filename();
    if (!copyIfExists(script, bundled_script, error)) {
      std::cerr << "machinelab: script copy failed: " << error << "\n";
      return 1;
    }
  }

  copyTreeIfExists(fs::path(MACHINE_LAB_SOURCE_DIR) / "sdk" / "include",
                   output / "sdk" / "include",
                   error);
  fs::path static_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "libmachinelab.a";
  if (!fs::exists(static_lib, ec)) static_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "lib" / "libmachinelab.a";
  if (!fs::exists(static_lib, ec)) static_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "liblcom.a";
  if (!fs::exists(static_lib, ec)) static_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "liblowlab.a";
  copyIfExists(static_lib, output / "sdk" / "lib" / "libmachinelab.a", error);

  std::string script_arg;
  if (!bundled_script.empty()) {
    script_arg = " --script \"$DIR/scripts/" + bundled_script.filename().string() + "\"";
  }
  std::string realtime_arg = opts.realtime ? " --realtime" : " --no-realtime";
  std::string run_args = "run --display " + opts.display + " --audio " + opts.audio +
                         realtime_arg + script_arg +
                         " -- \"$DIR/app/" + bundled_program.filename().string() + "\" \"$@\"";

  fs::path run_sh = output / "run.sh";
  {
    std::ofstream out(run_sh);
    out << "#!/usr/bin/env sh\n"
        << "set -eu\n"
        << "DIR=$(CDPATH= cd -- \"$(dirname -- \"$0\")\" && pwd)\n"
        << "exec \"$DIR/bin/" << bundled_runtime.filename().string() << "\" " << run_args << "\n";
  }
  makeExecutable(run_sh);

  fs::path run_bat = output / "run.bat";
  {
    std::ofstream out(run_bat);
    out << "@echo off\r\n"
        << "set DIR=%~dp0\r\n"
        << "\"%DIR%bin\\" << bundled_runtime.filename().string() << "\" run --display "
        << opts.display << " --audio " << opts.audio << " "
        << (opts.realtime ? "--realtime" : "--no-realtime");
    if (!bundled_script.empty()) {
      out << " --script \"%DIR%scripts\\" << bundled_script.filename().string() << "\"";
    }
    out << " -- \"%DIR%app\\" << bundled_program.filename().string() << "\" %*\r\n";
  }

  {
    std::ofstream out(output / "bundle.json");
    out << "{\n"
        << "  \"name\": \"" << jsonEscape(opts.name) << "\",\n"
        << "  \"runtime\": \"bin/" << jsonEscape(bundled_runtime.filename().string()) << "\",\n"
        << "  \"program\": \"app/" << jsonEscape(bundled_program.filename().string()) << "\",\n"
        << "  \"display\": \"" << jsonEscape(opts.display) << "\",\n"
        << "  \"audio\": \"" << jsonEscape(opts.audio) << "\",\n"
        << "  \"realtime\": " << (opts.realtime ? "true" : "false") << ",\n"
        << "  \"script\": \"" << (bundled_script.empty() ? "" : jsonEscape("scripts/" + bundled_script.filename().string())) << "\"\n"
        << "}\n";
  }

  {
    std::ofstream out(output / "README.md");
    out << "# " << opts.name << "\n\n"
        << "Run this bundled Machine Lab app with:\n\n"
        << "```sh\n./run.sh\n```\n\n"
        << "The `sdk/` directory contains the public headers and `libmachinelab.a` when the bundle command can find it.\n";
  }

  return 0;
}

static int bundleProject(const BundleOptions &opts, const char *argv0) {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::path output = fs::absolute(opts.output_path);

  if (opts.format == "dir") {
    int rc = createBundleDirectory(opts, argv0, output);
    if (rc == 0) std::cout << "Created Machine Lab bundle directory at " << output << "\n";
    return rc;
  }

  fs::path stage = output.parent_path() / ("." + output.filename().string() + ".staging");
  fs::remove_all(stage, ec);
  fs::create_directories(stage, ec);
  if (ec) {
    std::cerr << "machinelab: could not create staging directory: " << ec.message() << "\n";
    return 1;
  }

  int rc = createBundleDirectory(opts, argv0, stage);
  if (rc != 0) {
    fs::remove_all(stage, ec);
    return rc;
  }

  std::string error;
  if (!writeSingleFileBundle(stage, output, error)) {
    std::cerr << "machinelab: could not create single-file bundle: " << error << "\n";
    fs::remove_all(stage, ec);
    return 1;
  }
  fs::remove_all(stage, ec);
  std::cout << "Created Machine Lab bundle executable at " << output << "\n";
  return 0;
}

static std::string labStub(const std::string &lab, const std::string &source) {
  if (lab == "lab1" && source == "bitwise.c") {
    return R"(#include "bitwise.h"

uint8_t bit_clear(uint8_t value, uint8_t bit) {
  (void)value;
  (void)bit;
  /* TODO: clear bit `bit` from value. */
  return 0;
}

uint8_t bit_set(uint8_t value, uint8_t bit) {
  (void)value;
  (void)bit;
  /* TODO: set bit `bit` in value. */
  return 0;
}

int bit_is_set(uint8_t value, uint8_t bit) {
  (void)value;
  (void)bit;
  /* TODO: return non-zero when bit `bit` is set. */
  return 0;
}

uint8_t bit_lsb(uint16_t value) {
  (void)value;
  /* TODO: return the least significant byte. */
  return 0;
}

uint8_t bit_msb(uint16_t value) {
  (void)value;
  /* TODO: return the most significant byte. */
  return 0;
}

uint8_t bit_mask(unsigned first_bit, ...) {
  (void)first_bit;
  /* TODO: build a mask from all varargs until BIT_MASK_END. */
  return 0;
}
)";
  }
  if (lab == "lab1" && source == "rtc_lab.c") {
    return R"(#include "rtc_lib.h"

int rtc_read_date(lcom_rtc_date_t *date) {
  (void)date;
  /* TODO: read RTC day/month/year through RTC_ADDR_REG and RTC_DATA_REG. */
  return LCOM_ERR;
}

int rtc_read_time(lcom_rtc_time_t *time) {
  (void)time;
  /* TODO: read RTC seconds/minutes/hours through RTC_ADDR_REG and RTC_DATA_REG. */
  return LCOM_ERR;
}
)";
  }
  if (lab == "lab2" && source == "timer_lab.c") {
    return R"(#include "timer_lib.h"

int timer_set_frequency(uint8_t timer, uint32_t freq) {
  (void)timer;
  (void)freq;
  /* TODO: program the i8254 control word and divisor bytes. */
  return LCOM_ERR;
}

int timer_get_conf(uint8_t timer, uint8_t *status) {
  (void)timer;
  (void)status;
  /* TODO: issue a read-back command and read the selected timer status. */
  return LCOM_ERR;
}

int timer_subscribe(lcom_irq_t *irq) {
  (void)irq;
  /* TODO: subscribe TIMER0_IRQ. */
  return LCOM_ERR;
}

int timer_unsubscribe(lcom_irq_t *irq) {
  (void)irq;
  /* TODO: unsubscribe the timer IRQ. */
  return LCOM_ERR;
}

void timer_ih(void) {
  /* TODO: update your timer interrupt counter. */
}

uint32_t timer_ticks(void) {
  /* TODO: return your timer interrupt counter. */
  return 0;
}
)";
  }
  if (lab == "lab3" && source == "keyboard_lab.c") {
    return R"(#include "keyboard_lib.h"

int kbc_read_status(uint8_t *status) {
  (void)status;
  /* TODO: read KBC_ST_REG. */
  return LCOM_ERR;
}

int kbc_read_output(uint8_t *byte) {
  (void)byte;
  /* TODO: read KBC_OUT_BUF after checking status. */
  return LCOM_ERR;
}

int kbc_write_command(uint8_t command) {
  (void)command;
  /* TODO: write a command to KBC_CMD_REG. */
  return LCOM_ERR;
}

int kbd_process_byte(uint8_t byte) {
  (void)byte;
  /* TODO: assemble one-byte and two-byte scancodes. */
  return 0;
}

int kbd_get_scancode(uint8_t bytes[2], uint8_t *size, int *make) {
  (void)bytes;
  (void)size;
  (void)make;
  /* TODO: copy the latest complete scancode. */
  return LCOM_ERR;
}
)";
  }
  if (lab == "lab4" && source == "mouse_lab.c") {
    return R"(#include "mouse_lib.h"

int mouse_enable_data_reporting(void) {
  /* TODO: send MOUSE_CMD_ENABLE_DR through the i8042 mouse command path. */
  return LCOM_ERR;
}

int mouse_disable_data_reporting(void) {
  /* TODO: send MOUSE_CMD_DISABLE_DR through the i8042 mouse command path. */
  return LCOM_ERR;
}

int mouse_process_byte(uint8_t byte) {
  (void)byte;
  /* TODO: synchronize and assemble PS/2 three-byte packets. */
  return 0;
}

int mouse_get_packet(mouse_packet_t *packet) {
  (void)packet;
  /* TODO: decode buttons, signed deltas, and raw bytes from the latest packet. */
  return LCOM_ERR;
}
)";
  }
  if (lab == "lab5" && source == "graphics_lab.c") {
    return R"(#include "video_lib.h"

int video_set_mode(uint16_t mode) {
  (void)mode;
  /* TODO: query mode info and switch VBE mode. */
  return LCOM_ERR;
}

int video_map_framebuffer(void) {
  /* TODO: map the VBE framebuffer with lcom_phys_map. */
  return LCOM_ERR;
}

int video_fill_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint32_t color) {
  (void)x;
  (void)y;
  (void)w;
  (void)h;
  (void)color;
  /* TODO: write pixels into the mapped framebuffer. */
  return LCOM_ERR;
}

int video_draw_xpm(const char *const *xpm, int16_t x, int16_t y) {
  (void)xpm;
  (void)x;
  (void)y;
  /* TODO: parse a simple XPM and draw it with clipping and transparency. */
  return LCOM_ERR;
}

int video_present(void) {
  /* TODO: present the framebuffer to the runtime display. */
  return LCOM_ERR;
}
)";
  }
  if (lab == "lab6" && source == "audio_lab.c") {
    return R"(#include "audio_lib.h"

int audio_map_buffer(void) {
  /* TODO: get and map the AC97-lite PCM buffer. */
  return LCOM_ERR;
}

int audio_fill_square_wave(uint32_t hz, uint32_t ms) {
  (void)hz;
  (void)ms;
  /* TODO: write signed 16-bit stereo PCM samples into the mapped buffer. */
  return LCOM_ERR;
}

int audio_play(size_t byte_count) {
  (void)byte_count;
  /* TODO: program AC97-lite playback registers and set RUN. */
  return LCOM_ERR;
}

int audio_stop(void) {
  /* TODO: clear AC97-lite playback RUN. */
  return LCOM_ERR;
}
)";
  }
  if (lab == "lab7" && source == "uart_lab.c") {
    return R"(#include "uart_lib.h"

int uart_config(uint16_t base, uint32_t baud, uint8_t line_control) {
  (void)base;
  (void)baud;
  (void)line_control;
  /* TODO: configure the 16550 divisor latch and line control register. */
  return LCOM_ERR;
}

int uart_enable_fifo(uint16_t base) {
  (void)base;
  /* TODO: enable and clear RX/TX FIFOs. */
  return LCOM_ERR;
}

int uart_enable_rx_interrupt(uint16_t base) {
  (void)base;
  /* TODO: enable received-data-available interrupts. */
  return LCOM_ERR;
}

int uart_set_loopback(uint16_t base, int enabled) {
  (void)base;
  (void)enabled;
  /* TODO: update MCR loopback mode. */
  return LCOM_ERR;
}

int uart_send_byte(uint16_t base, uint8_t byte) {
  (void)base;
  (void)byte;
  /* TODO: wait for THR empty and write one byte. */
  return LCOM_ERR;
}

int uart_read_byte(uint16_t base, uint8_t *byte) {
  (void)base;
  (void)byte;
  /* TODO: check RX ready and read one byte. */
  return LCOM_ERR;
}

int uart_subscribe(uint8_t irq, lcom_irq_t *out) {
  (void)irq;
  (void)out;
  /* TODO: subscribe the requested UART IRQ. */
  return LCOM_ERR;
}

int uart_unsubscribe(lcom_irq_t *irq) {
  (void)irq;
  /* TODO: unsubscribe the UART IRQ. */
  return LCOM_ERR;
}
)";
  }
  return "";
}

static std::string studentMakefileText(const std::filesystem::path &root) {
  std::ostringstream out;
  out << "MACHINELAB_ROOT ?= " << root.string() << "\n"
      << "MACHINELAB_BUILD ?= $(MACHINELAB_ROOT)/build\n"
      << "MACHINELAB ?= machinelab\n"
      << "CC ?= cc\n"
      << "BUILD_DIR := build\n"
      << "OBJ_DIR := $(BUILD_DIR)/obj\n"
      << "MACHINELAB_LIB := $(MACHINELAB_BUILD)/libmachinelab.a\n"
      << "CPPFLAGS += -Iinclude -Ilib -I$(MACHINELAB_ROOT)/sdk/include";
  for (const StudentLabSpec &spec : studentLabSpecs()) {
    out << " -Ilabs/" << spec.dir << "/include";
  }
  out << "\n"
      << "CFLAGS += -std=c11 -Wall -Wextra -Wpedantic\n\n"
      << "LAB_SOURCES :=";
  for (const StudentLabSpec &spec : studentLabSpecs()) {
    for (const std::string &source : spec.sources) {
      out << " labs/" << spec.dir << "/" << source;
    }
  }
  out << "\n"
      << "PROJ_SOURCES := $(wildcard proj/*.c)\n"
      << "LAB_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(LAB_SOURCES))\n"
      << "PROJ_OBJECTS := $(patsubst %.c,$(OBJ_DIR)/%.o,$(PROJ_SOURCES))\n"
      << "PROJ_BIN := $(BUILD_DIR)/student_project\n\n"
      << ".PHONY: all labs proj run clean\n\n"
      << "all: proj\n\n"
      << "labs: $(LAB_OBJECTS)\n\n"
      << "proj: $(PROJ_BIN)\n\n"
      << "$(PROJ_BIN): $(PROJ_OBJECTS) $(LAB_OBJECTS) $(MACHINELAB_LIB)\n"
      << "\t@mkdir -p $(dir $@)\n"
      << "\t$(CC) $(CFLAGS) -o $@ $(PROJ_OBJECTS) $(LAB_OBJECTS) $(MACHINELAB_LIB)\n\n"
      << "$(OBJ_DIR)/%.o: %.c\n"
      << "\t@mkdir -p $(dir $@)\n"
      << "\t$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@\n\n"
      << "run: proj\n"
      << "\t$(MACHINELAB) run $(PROJ_BIN)\n\n"
      << "clean:\n"
      << "\trm -rf $(BUILD_DIR)\n";
  return out.str();
}

static std::string starterLibraryHeader(const std::string &device) {
  std::string guard = "LCOM_STUDENT_LIB_" + device + "_H";
  std::transform(guard.begin(), guard.end(), guard.begin(), [](unsigned char c) {
    return static_cast<char>(std::toupper(c == '-' ? '_' : c));
  });

  std::ostringstream out;
  out << "#ifndef " << guard << "\n"
      << "#define " << guard << "\n\n"
      << "/*\n"
      << " * Optional helper library for the " << device << " area.\n"
      << " * Put reusable declarations here; the tested lab entrypoints live under labs/.\n"
      << " */\n\n";
  if (device == "rtc") {
    out << "#include <lcom/rtc.h>\n#include <stdint.h>\n\n"
        << "int rtc_read_register(uint8_t reg, uint8_t *value);\n";
  } else if (device == "timer") {
    out << "#include <lcom/i8254.h>\n#include <stdint.h>\n\n"
        << "uint16_t timer_divisor_for(uint32_t freq);\n";
  } else if (device == "kbc") {
    out << "#include <lcom/i8042.h>\n#include <stdint.h>\n\n"
        << "int kbc_wait_output(uint8_t *status);\n";
  } else if (device == "kbd") {
    out << "#include <stdint.h>\n\n"
        << "int kbd_is_make(uint8_t scancode);\n";
  } else if (device == "mouse") {
    out << "#include <lcom/i8042.h>\n#include <stdint.h>\n\n"
        << "int mouse_sync_byte(uint8_t byte);\n";
  } else if (device == "graphics") {
    out << "#include <lcom/vbe.h>\n#include <stdint.h>\n\n"
        << "uint32_t rgb(uint8_t r, uint8_t g, uint8_t b);\n";
  } else if (device == "audio") {
    out << "#include <lcom/ac97.h>\n#include <stdint.h>\n\n"
        << "int16_t audio_square_sample(uint32_t frame, uint32_t period);\n";
  } else if (device == "uart") {
    out << "#include <lcom/uart16550.h>\n#include <stdint.h>\n\n"
        << "int uart_wait_tx_ready(uint16_t base);\n";
  }
  out << "\n#endif\n";
  return out.str();
}

static std::string studentProjMainText() {
  return R"(#include <lcom/lcom.h>

int main(void) {
  if (lcom_init() != LCOM_OK) return 1;
  lcom_printf("hello from the Machine Lab student project\n");
  lcom_exit();
  return 0;
}
)";
}

static std::string studentWorkspaceReadmeText() {
  return R"(# Machine Lab Student Workspace

This is the workspace you edit for labs and projects. It was generated by
`machinelab setup`.

## Edit These

- `labs/<device>/*_lab.c`: required lab implementations.
- `lib/<device>/`: optional reusable helpers.
- `proj/`: experiments and final-project code.
- `Makefile`: build wiring, only when needed.

## Usually Ignore These

- `include/lcom/`: copied Machine Lab SDK headers; treat as read-only.
- `.mlab/`: copied tests and generated test builds.
- `build/`: generated object files and binaries.
- upstream `runtime/`, `common/`, `.github/`: Machine Lab developer internals.

## Commands

```sh
make
machinelab test rtc
machinelab test uart
make run
```

The lab folders are `rtc`, `timer`, `kbd`, `mouse`, `graphics`, `audio`, and
`uart`. You can also use `lab1` through `lab7` with `machinelab test`.
)";
}

static int setupStudentProject(const SetupOptions &opts) {
  namespace fs = std::filesystem;
  std::error_code ec;
  fs::path project = fs::absolute(opts.project_dir);
  fs::create_directories(project, ec);
  if (ec) {
    std::cerr << "machinelab: could not create " << project << ": " << ec.message() << "\n";
    return 1;
  }

  std::string error;
  if (!copyTreeIfExists(fs::path(MACHINE_LAB_SOURCE_DIR) / "sdk" / "include" / "lcom",
                        project / "include" / "lcom",
                        error) ||
      !copyTreeIfExists(fs::path(MACHINE_LAB_SOURCE_DIR) / "dev" / "lab-tests",
                        project / ".mlab" / "tests" / "labs",
                        error)) {
    std::cerr << "machinelab: setup copy failed: " << error << "\n";
    return 1;
  }

  for (const StudentLabSpec &spec : studentLabSpecs()) {
    if (!copyTreeIfExists(fs::path(MACHINE_LAB_SOURCE_DIR) / "course" / "labs" / "templates" / spec.id / "include",
                          project / "labs" / spec.dir / "include",
                          error)) {
      std::cerr << "machinelab: setup copy failed: " << error << "\n";
      return 1;
    }
    for (const std::string &source : spec.sources) {
      fs::path target = project / "labs" / spec.dir / source;
      if (!writeTextFile(target, labStub(spec.id, source), opts.force, error)) {
        std::cerr << "machinelab: setup write failed: " << error << "\n";
        return 1;
      }
    }
  }

  const std::vector<std::string> library_devices = {
      "rtc", "timer", "kbc", "kbd", "mouse", "graphics", "audio", "uart"};
  for (const std::string &device : library_devices) {
    if (!writeTextFile(project / "lib" / device / (device + ".h"),
                       starterLibraryHeader(device), opts.force, error)) {
      std::cerr << "machinelab: setup write failed: " << error << "\n";
      return 1;
    }
  }

  if (!writeTextFile(project / "proj" / "main.c", studentProjMainText(), opts.force, error)) {
    std::cerr << "machinelab: setup write failed: " << error << "\n";
    return 1;
  }

  if (opts.force) {
    fs::remove(project / "CMakeLists.txt", ec);
  }

  if (!writeTextFile(project / "Makefile", studentMakefileText(fs::path(MACHINE_LAB_SOURCE_DIR)), opts.force, error) ||
      !writeTextFile(project / "README.md", studentWorkspaceReadmeText(), opts.force, error)) {
    std::cerr << "machinelab: setup write failed: " << error << "\n";
    return 1;
  }

  std::cout << "Created Machine Lab student workspace at " << project << "\n";
  std::cout << "Try: machinelab test rtc --project " << project << "\n";
  return 0;
}

static std::string runtimeCommand(const char *argv0) {
  std::filesystem::path path(argv0);
  if (path.has_parent_path()) path = std::filesystem::absolute(path);
  return shellQuote(path);
}

static int runStudentLabTest(const TestOptions &opts, const char *argv0) {
  namespace fs = std::filesystem;
  const StudentLabSpec *spec = studentLabSpec(opts.lab);
  if (spec == nullptr) {
    std::cerr << "machinelab: unknown lab " << opts.lab << "\n";
    return 1;
  }

  std::error_code ec;
  fs::path project = fs::absolute(opts.project_dir);
  fs::path lab_dir = project / "labs" / spec->dir;
  if (!fs::exists(lab_dir, ec)) {
    std::cerr << "machinelab: " << spec->dir << " not found in " << project
              << "; run `machinelab setup` first\n";
    return 1;
  }
  if (std::system("command -v ${CC:-cc} >/dev/null 2>&1") != 0) {
    std::cerr << "machinelab: no C compiler found on PATH; set CC or install cc/clang/gcc\n";
    return 1;
  }

  fs::path lcom_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "libmachinelab.a";
  if (!fs::exists(lcom_lib, ec)) lcom_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "lib" / "libmachinelab.a";
  if (!fs::exists(lcom_lib, ec)) lcom_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "liblcom.a";
  if (!fs::exists(lcom_lib, ec)) lcom_lib = fs::path(MACHINE_LAB_BINARY_DIR) / "liblowlab.a";
  if (!fs::exists(lcom_lib, ec)) {
    std::cerr << "machinelab: could not find libmachinelab.a under " << MACHINE_LAB_BINARY_DIR
              << "; build Machine Lab first\n";
    return 1;
  }

  fs::path work_root = project / ".mlab" / "build" / spec->dir;
  fs::path output_dir = project / ".mlab" / "test-output";
  fs::remove_all(work_root, ec);
  fs::create_directories(work_root, ec);
  fs::create_directories(output_dir, ec);
  if (ec) {
    std::cerr << "machinelab: could not prepare test build: " << ec.message() << "\n";
    return 1;
  }

  const char *cc_env = std::getenv("CC");
  std::string cc = (cc_env != nullptr && cc_env[0] != '\0') ? cc_env : "cc";
  fs::path binary = work_root / ("student_" + spec->id + "_test");
  std::ostringstream compile;
  compile << cc << " -std=c11 -Wall -Wextra -Wpedantic"
          << " -I" << shellQuote(project / ".mlab" / "tests" / "labs")
          << " -I" << shellQuote(project / "include")
          << " -I" << shellQuote(project / "lib")
          << " -I" << shellQuote(lab_dir / "include")
          << " -I" << shellQuote(fs::path(MACHINE_LAB_SOURCE_DIR) / "sdk" / "include")
          << " " << shellQuote(project / ".mlab" / "tests" / "labs" / (spec->id + "_test.c"));
  for (const std::string &source : spec->sources) {
    compile << " " << shellQuote(lab_dir / source);
  }
  compile << " " << shellQuote(lcom_lib)
          << " -o " << shellQuote(binary);

  std::cout << "Building " << spec->dir << " predefined tests...\n";
  std::cout.flush();
  if (std::system(compile.str().c_str()) != 0) return 1;

  std::string run = runtimeCommand(argv0) + " run --headless";
  if (spec->id == "lab1") {
    run += " --rtc 2026-06-16T12:34:56";
  } else if (spec->id == "lab3") {
    run += " --script " + shellQuote(fs::path(MACHINE_LAB_SOURCE_DIR) / "scripts" / "type_a_esc.mlabscript");
  } else if (spec->id == "lab4") {
    run += " --script " + shellQuote(fs::path(MACHINE_LAB_SOURCE_DIR) / "scripts" / "mouse_move.mlabscript");
  } else if (spec->id == "lab5") {
    run += " --dump-frame " + shellQuote(output_dir / "graphics.ppm");
  } else if (spec->id == "lab6") {
    run += " --audio-wav " + shellQuote(output_dir / "audio.wav");
  }
  run += " -- " + shellQuote(binary);

  std::cout << "Running " << spec->dir << " predefined tests...\n";
  std::cout.flush();
  int rc = std::system(run.c_str());
  if (!opts.keep_build) fs::remove_all(work_root, ec);
  return rc == 0 ? 0 : 1;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
    return 1;
  }

  std::string cmd = argv[1];
  RuntimeOptions opts = defaultRuntimeOptions();

  const bool known_subcommand =
      cmd == "run" || cmd == "run-pair" || cmd == "replay" || cmd == "setup" ||
      cmd == "test" || cmd == "bundle" || cmd == "lab" || cmd == "docs" ||
      cmd == "completion";
  if (known_subcommand && argc >= 3 &&
      (std::string(argv[2]) == "--help" || std::string(argv[2]) == "-h")) {
    subcommandUsage(cmd);
    return 0;
  }

  if (cmd == "run") {
    if (!parseRun(argc, argv, 2, opts)) {
      std::cerr << "machinelab: run requires <program> [args...]\n";
      subcommandUsage("run");
      return 1;
    }
    applyRunDefaults(opts);
  } else if (cmd == "run-pair") {
    PairRuntimeOptions pair = defaultPairRuntimeOptions();
    if (!parseRunPair(argc, argv, 2, pair)) {
      std::cerr << "machinelab: run-pair requires <left-program> [args...] --right <right-program> [args...]\n";
      subcommandUsage("run-pair");
      return 1;
    }
    PairRuntimeServer server(pair);
    return server.run();
  } else if (cmd == "replay") {
    if (!parseReplay(argc, argv, 2, opts)) {
      std::cerr << "machinelab: replay requires <script> <program> [args...]\n";
      subcommandUsage("replay");
      return 1;
    }
    opts.guest_input = false;
    if (!opts.realtime && !opts.no_realtime) opts.no_realtime = true;
    applyRunDefaults(opts);
  } else if (cmd == "setup") {
    SetupOptions setup;
    if (!parseSetup(argc, argv, 2, setup)) {
      subcommandUsage("setup");
      return 1;
    }
    return setupStudentProject(setup);
  } else if (cmd == "test") {
    TestOptions test;
    if (!parseTest(argc, argv, 2, test)) {
      std::cerr << "machinelab: test requires <lab>\n";
      subcommandUsage("test");
      return 1;
    }
    return runStudentLabTest(test, argv[0]);
  } else if (cmd == "bundle") {
    BundleOptions bundle;
    if (!parseBundle(argc, argv, 2, bundle)) {
      subcommandUsage("bundle");
      return 1;
    }
    return bundleProject(bundle, argv[0]);
  } else if (cmd == "docs") {
    if (argc >= 3 && std::string(argv[2]) == "cli") return cliDocs();
    std::cerr << "machinelab: docs requires a subcommand, e.g. 'cli'\n";
    subcommandUsage("docs");
    return 1;
  } else if (cmd == "completion") {
    if (argc >= 3) return completionScript(argv[2]);
    std::cerr << "machinelab: completion requires <shell> (bash, zsh, or fish)\n";
    subcommandUsage("completion");
    return 1;
  } else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
    usage();
    return 0;
  } else if (cmd == "lab") {
    if (argc >= 3 && std::string(argv[2]) == "list") return labList();
    if (argc >= 4 && std::string(argv[2]) == "show") return labShow(argv[3]);
    std::cerr << "machinelab: lab requires 'list' or 'show <lab>'\n";
    subcommandUsage("lab");
    return 1;
  } else {
    std::cerr << "machinelab: unknown command " << cmd << "\n";
    usage();
    return 1;
  }

  RuntimeServer server(opts);
  return server.run();
}
