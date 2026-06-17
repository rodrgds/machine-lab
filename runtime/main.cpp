#include "core/RuntimeServer.hpp"

#include <CLI/CLI.hpp>

#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

using lcom::RuntimeOptions;
using lcom::RuntimeServer;

static void configureCliApp(CLI::App &app) {
  app.description("lcom-ng userspace machine runtime");
  app.require_subcommand(0, 1);

  auto *run = app.add_subcommand("run", "Run a student program inside the lcom-ng runtime");
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

  auto *replay = app.add_subcommand("replay", "Replay a script against a student program");
  replay->add_option("script", "Script file")->required();
  replay->add_option("program", "Program and arguments after runtime options");
  replay->add_option("--display", "Display backend: headless or sdl");
  replay->add_option("--audio", "Audio backend: null, sdl, or wav:<path>");
  replay->add_option("--video", "Render presented frames to MP4 with ffmpeg");
  replay->add_option("--video-fps", "FPS for --video output");

  auto *bundle = app.add_subcommand("bundle", "Create a runnable lcom-ng app bundle");
  bundle->add_option("project-dir", "Project directory")->required();
  bundle->add_option("--program", "Student binary to bundle")->required();
  bundle->add_option("--name", "Bundle/app name");
  bundle->add_option("--output", "Output bundle directory");
  bundle->add_option("--script", "Script to include and run with the app");
  bundle->add_option("--display", "Display backend in generated launcher");
  bundle->add_option("--audio", "Audio backend in generated launcher");
  bundle->add_flag("--headless", "Generate a headless launcher");
  bundle->add_flag("--realtime", "Generate a realtime launcher");
  bundle->add_flag("--no-realtime", "Generate a non-realtime launcher");

  auto *lab = app.add_subcommand("lab", "Inspect lab function requests");
  lab->add_subcommand("list", "List labs");
  auto *show = lab->add_subcommand("show", "Show requested functions for a lab");
  show->add_option("lab", "lab1 through lab6")->required();

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

struct BundleOptions {
  std::filesystem::path project_dir;
  std::filesystem::path program;
  std::filesystem::path output_dir;
  std::filesystem::path script;
  std::string name;
  std::string display = "sdl";
  std::string audio = "sdl";
  bool realtime = true;
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
  std::cout
      << "lab1  Bitwise helpers and RTC/CMOS\n"
      << "lab2  i8254 PIT and IRQ0 timer events\n"
      << "lab3  i8042 keyboard and PS/2 scancodes\n"
      << "lab4  PS/2 mouse packets\n"
      << "lab5  VBE framebuffer graphics and XPM sprites\n"
      << "lab6  AC97-lite PCM audio\n";
  return 0;
}

static int labShow(const std::string &lab) {
  if (lab == "lab1") {
    std::cout << "lab1: bit_clear, bit_set, bit_is_set, bit_lsb, bit_msb, bit_mask, rtc_read_date, rtc_read_time\n";
  } else if (lab == "lab2") {
    std::cout << "lab2: timer_set_frequency, timer_get_conf, timer_subscribe, timer_unsubscribe, timer_ih, timer_ticks\n";
  } else if (lab == "lab3") {
    std::cout << "lab3: kbc_read_status, kbc_read_output, kbc_write_command, kbd_process_byte, kbd_get_scancode\n";
  } else if (lab == "lab4") {
    std::cout << "lab4: mouse_enable_data_reporting, mouse_disable_data_reporting, mouse_process_byte, mouse_get_packet\n";
  } else if (lab == "lab5") {
    std::cout << "lab5: video_set_mode, video_map_framebuffer, video_fill_rect, video_draw_xpm, video_present\n";
  } else if (lab == "lab6") {
    std::cout << "lab6: audio_map_buffer, audio_fill_square_wave, audio_play, audio_stop\n";
  } else {
    std::cerr << "lcom: unknown lab " << lab << "\n";
    return 1;
  }
  return 0;
}

static int cliDocs() {
  CLI::App app;
  configureCliApp(app);
  std::cout << "# lcom CLI\n\n";
  std::cout << "```text\n" << app.help() << "```\n";
  std::cout << "\nCommon examples:\n\n";
  std::cout << "```sh\n";
  std::cout << "lcom run --display sdl build/examples/flappy_bird\n";
  std::cout << "lcom replay scripts/flappy_mouse_demo.lcomscript --headless --video build/flappy.mp4 -- build/examples/flappy_bird\n";
  std::cout << "lcom bundle . --program build/examples/flappy_bird --name flappy-bird\n";
  std::cout << "lcom completion zsh > _lcom\n";
  std::cout << "```\n";
  return 0;
}

static int completionScript(const std::string &shell) {
  const char *commands = "run replay bundle lab docs completion help";
  const char *run_opts =
      "--headless --display --audio --audio-wav --realtime --no-realtime --fullscreen "
      "--scale --integer-scale --script --trace --dump-frame --frame-dir --video --video-fps "
      "--rtc --max-ticks";
  const char *bundle_opts =
      "--program --name --output --script --display --audio --headless --realtime --no-realtime";

  if (shell == "bash") {
    std::cout
        << "_lcom_complete() {\n"
        << "  local cur prev cmd\n"
        << "  COMPREPLY=()\n"
        << "  cur=\"${COMP_WORDS[COMP_CWORD]}\"\n"
        << "  cmd=\"${COMP_WORDS[1]}\"\n"
        << "  if [ $COMP_CWORD -eq 1 ]; then COMPREPLY=( $(compgen -W \"" << commands << "\" -- \"$cur\") ); return 0; fi\n"
        << "  case \"$cmd\" in\n"
        << "    run|replay) COMPREPLY=( $(compgen -W \"" << run_opts << "\" -- \"$cur\") ) ;;\n"
        << "    bundle) COMPREPLY=( $(compgen -W \"" << bundle_opts << "\" -- \"$cur\") ) ;;\n"
        << "    lab) COMPREPLY=( $(compgen -W \"list show lab1 lab2 lab3 lab4 lab5 lab6\" -- \"$cur\") ) ;;\n"
        << "    completion) COMPREPLY=( $(compgen -W \"bash zsh fish\" -- \"$cur\") ) ;;\n"
        << "  esac\n"
        << "}\ncomplete -F _lcom_complete lcom\n";
    return 0;
  }
  if (shell == "zsh") {
    std::cout
        << "#compdef lcom\n"
        << "_lcom() {\n"
        << "  local -a commands\n"
        << "  commands=(run replay bundle lab docs completion help)\n"
        << "  if (( CURRENT == 2 )); then _describe 'command' commands; return; fi\n"
        << "  case $words[2] in\n"
        << "    run|replay) _arguments '--headless' '--display[backend]:' '--audio[backend]:' '--audio-wav[file]:file:_files' '--realtime' '--no-realtime' '--fullscreen' '--scale[n]:' '--integer-scale' '--script[file]:file:_files' '--trace[file]:file:_files' '--dump-frame[file]:file:_files' '--frame-dir[dir]:dir:_files -/' '--video[file]:file:_files' '--video-fps[n]:' '--rtc[iso-time]:' '--max-ticks[n]:' '*::program:_files' ;;\n"
        << "    bundle) _arguments '--program[student binary]:file:_files' '--name[name]:' '--output[dir]:dir:_files -/' '--script[script]:file:_files' '--display[backend]:' '--audio[backend]:' '--headless' '--realtime' '--no-realtime' ;;\n"
        << "    lab) _arguments '1:action:(list show)' '2:lab:(lab1 lab2 lab3 lab4 lab5 lab6)' ;;\n"
        << "    completion) _arguments '1:shell:(bash zsh fish)' ;;\n"
        << "  esac\n"
        << "}\n_lcom \"$@\"\n";
    return 0;
  }
  if (shell == "fish") {
    std::cout
        << "complete -c lcom -f -n '__fish_use_subcommand' -a '" << commands << "'\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from run replay' -l display -r\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from run replay' -l audio -r\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from run replay' -l realtime\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from run replay' -l no-realtime\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from run replay' -l script -r\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from run replay' -l video -r\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from bundle' -l program -r\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from bundle' -l output -r\n"
        << "complete -c lcom -n '__fish_seen_subcommand_from completion' -a 'bash zsh fish'\n";
    return 0;
  }
  std::cerr << "lcom: unknown completion shell " << shell << "\n";
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
      std::cerr << "lcom: unknown option " << arg << "\n";
      return false;
    } else {
      for (int j = i; j < argc; j++) opts.program.emplace_back(argv[j]);
      return !opts.program.empty();
    }
  }
  return false;
}

static void applyRunDefaults(RuntimeOptions &opts) {
  if (!opts.headless && !opts.realtime && !opts.no_realtime) opts.realtime = true;
}

static RuntimeOptions defaultRuntimeOptions() {
  RuntimeOptions opts;
#if defined(LCOM_WITH_SDL)
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

static bool parseBundle(int argc, char **argv, int start, BundleOptions &opts) {
  if (start >= argc) return false;
  opts.project_dir = argv[start++];

  for (int i = start; i < argc; i++) {
    std::string arg = argv[i];
    if (arg == "--program" && i + 1 < argc) {
      opts.program = argv[++i];
    } else if (arg == "--name" && i + 1 < argc) {
      opts.name = argv[++i];
    } else if (arg == "--output" && i + 1 < argc) {
      opts.output_dir = argv[++i];
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
    } else {
      std::cerr << "lcom: unknown bundle option " << arg << "\n";
      return false;
    }
  }

  if (opts.program.empty()) {
    std::cerr << "lcom: bundle requires --program <binary>\n";
    return false;
  }
  if (opts.name.empty()) opts.name = opts.program.filename().string();
  if (opts.output_dir.empty()) {
    opts.output_dir = opts.project_dir / "dist" / (opts.name + ".lcom");
  }
  return true;
}

static bool copyIfExists(const std::filesystem::path &from,
                         const std::filesystem::path &to,
                         std::string &error) {
  std::error_code ec;
  if (!std::filesystem::exists(from, ec)) return true;
  std::filesystem::create_directories(to.parent_path(), ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  std::filesystem::copy_file(from, to, std::filesystem::copy_options::overwrite_existing, ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  return true;
}

static bool copyTreeIfExists(const std::filesystem::path &from,
                             const std::filesystem::path &to,
                             std::string &error) {
  std::error_code ec;
  if (!std::filesystem::exists(from, ec)) return true;
  std::filesystem::create_directories(to, ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  std::filesystem::copy(from, to,
                        std::filesystem::copy_options::recursive |
                            std::filesystem::copy_options::overwrite_existing,
                        ec);
  if (ec) {
    error = ec.message();
    return false;
  }
  return true;
}

static void makeExecutable(const std::filesystem::path &path) {
  std::error_code ec;
  std::filesystem::permissions(path,
                               std::filesystem::perms::owner_exec |
                                   std::filesystem::perms::group_exec |
                                   std::filesystem::perms::others_exec,
                               std::filesystem::perm_options::add,
                               ec);
}

static int bundleProject(const BundleOptions &opts, const char *argv0) {
  namespace fs = std::filesystem;
  std::error_code ec;

  fs::path project = fs::absolute(opts.project_dir);
  fs::path program = fs::absolute(opts.program);
  fs::path output = fs::absolute(opts.output_dir);
  if (!fs::exists(project, ec) || !fs::is_directory(project, ec)) {
    std::cerr << "lcom: project directory does not exist: " << project << "\n";
    return 1;
  }
  if (!fs::exists(program, ec) || !fs::is_regular_file(program, ec)) {
    std::cerr << "lcom: program does not exist: " << program << "\n";
    return 1;
  }

  fs::create_directories(output / "bin", ec);
  fs::create_directories(output / "app", ec);
  fs::create_directories(output / "sdk" / "lib", ec);
  fs::create_directories(output / "scripts", ec);
  if (ec) {
    std::cerr << "lcom: could not create bundle: " << ec.message() << "\n";
    return 1;
  }

  std::string error;
  fs::path runtime = fs::absolute(argv0);
  fs::path bundled_runtime = output / "bin" / runtime.filename();
  fs::path bundled_program = output / "app" / program.filename();
  if (!copyIfExists(runtime, bundled_runtime, error) ||
      !copyIfExists(program, bundled_program, error)) {
    std::cerr << "lcom: bundle copy failed: " << error << "\n";
    return 1;
  }
  makeExecutable(bundled_runtime);
  makeExecutable(bundled_program);

  fs::path bundled_script;
  if (!opts.script.empty()) {
    fs::path script = fs::absolute(opts.script);
    bundled_script = output / "scripts" / script.filename();
    if (!copyIfExists(script, bundled_script, error)) {
      std::cerr << "lcom: script copy failed: " << error << "\n";
      return 1;
    }
  }

  copyTreeIfExists(fs::path(LCOM_SOURCE_DIR) / "include", output / "sdk" / "include", error);
  fs::path static_lib = fs::path(LCOM_BINARY_DIR) / "liblcom-ng.a";
  if (!fs::exists(static_lib, ec)) static_lib = fs::path(LCOM_BINARY_DIR) / "lib" / "liblcom-ng.a";
  copyIfExists(static_lib, output / "sdk" / "lib" / "liblcom-ng.a", error);

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
        << "Run this bundled lcom-ng app with:\n\n"
        << "```sh\n./run.sh\n```\n\n"
        << "The `sdk/` directory contains the public headers and `liblcom-ng.a` when the bundle command can find it.\n";
  }

  std::cout << "Created lcom bundle at " << output << "\n";
  return 0;
}

int main(int argc, char **argv) {
  if (argc < 2) {
    usage();
    return 1;
  }

  std::string cmd = argv[1];
  RuntimeOptions opts = defaultRuntimeOptions();

  if (cmd == "run") {
    if (!parseRun(argc, argv, 2, opts)) {
      usage();
      return 1;
    }
    applyRunDefaults(opts);
  } else if (cmd == "replay") {
    if (argc < 4) {
      usage();
      return 1;
    }
    opts.script_path = argv[2];
    if (!parseRun(argc, argv, 3, opts)) {
      usage();
      return 1;
    }
    applyRunDefaults(opts);
  } else if (cmd == "bundle") {
    BundleOptions bundle;
    if (!parseBundle(argc, argv, 2, bundle)) {
      usage();
      return 1;
    }
    return bundleProject(bundle, argv[0]);
  } else if (cmd == "docs") {
    if (argc >= 3 && std::string(argv[2]) == "cli") return cliDocs();
    usage();
    return 1;
  } else if (cmd == "completion") {
    if (argc >= 3) return completionScript(argv[2]);
    usage();
    return 1;
  } else if (cmd == "help" || cmd == "--help" || cmd == "-h") {
    usage();
    return 0;
  } else if (cmd == "lab") {
    if (argc >= 3 && std::string(argv[2]) == "list") return labList();
    if (argc >= 4 && std::string(argv[2]) == "show") return labShow(argv[3]);
    usage();
    return 1;
  } else {
    std::cerr << "lcom: unknown command " << cmd << "\n";
    usage();
    return 1;
  }

  RuntimeServer server(opts);
  return server.run();
}
