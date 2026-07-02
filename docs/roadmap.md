# Scope And Roadmap

## Implemented

- headless and SDL3 runtime backends;
- debug overlay, input history, tracing, screenshots, audio capture, replay
  recording, captions, capture cuts, and MP4 rendering;
- i8254, i8042, RTC/CMOS, VBE, AC97-lite, and UART16550 models;
- two-process UART bridging through `run-pair`;
- generated Makefile student workspaces and seven tested labs;
- focused device examples, three project seed apps, and three complete project ports;
- single-file and exploded application bundles;
- Bash, Zsh, and Fish completion plus generated CLI Markdown;
- GitHub Actions release artifacts for Linux, macOS x64, macOS arm64, and
  Windows/MSYS2 headless builds;
- unit, solution, and end-to-end integration coverage.

## Near-term

- generate browsable API/lab documentation from headers and lab metadata;
- report lab failures as structured grading output with linked traces/artifacts;
- add UART latency, loss, framing-error, and retransmission scenarios;
- expand RTC alarm/periodic interrupt behavior;
- add keyboard LED/command and richer mouse gesture exercises;
- add a browsable static course site generated from the lab handouts.

## Possible Extensions

- palette editing, page flipping, dirty rectangles, and text modes;
- more PC speaker and audio exercises;
- storage or network device sketches where they support clear learning goals;
- more small games and creative tools such as Pong, Tetris, paint, or trackers;
- platform-specific app wrappers and cross-compilation recipes.

New devices should only be added with a specific lab or project outcome. Broad
hardware emulation is not the goal.
