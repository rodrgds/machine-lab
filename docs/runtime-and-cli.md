# Runtime And CLI

The CLI is the supported entry point for running, testing, recording, and
packaging student programs. Use `machinelab <command> --help` for the authoritative
option list; this guide explains when to use each command.

## Commands

| Command | Purpose |
| --- | --- |
| `machinelab run` | host one student program |
| `machinelab run-pair` | host two programs with bridged UARTs |
| `machinelab replay` | run one program with an authored input timeline |
| `machinelab setup` | generate a student workspace |
| `machinelab test` | compile and execute one predefined student lab test |
| `machinelab lab` | list labs or inspect requested functions |
| `machinelab bundle` | package a runtime and program for the current platform |
| `machinelab docs cli` | print a Markdown CLI summary |
| `machinelab completion` | emit Bash, Zsh, or Fish completion |

## Interactive And Headless Runs

```sh
machinelab run build/examples/flappy_bird
machinelab run --headless --max-ticks 300 -- build/examples/timer_int 3
```

SDL runs advance virtual time from host time by default. Headless runs advance
deterministically as the guest waits for events. `--max-ticks` is a guard for
headless programs that do not terminate.

Useful output options:

```sh
machinelab run --headless --trace build/run.jsonl -- build/examples/timer_int 3
machinelab run --headless --dump-frame build/frame.ppm -- build/examples/vbe_rectangle
machinelab run --headless --audio-wav build/audio.wav -- build/examples/audio_tone
```

## SDL Controls

| Key | Action |
| --- | --- |
| F3 | toggle the runtime debug overlay |
| F8 | pause/resume capture while recording a replay |
| F9 | start replay recording |
| F10 | stop recording and choose a save path |
| F11 | toggle fullscreen |
| Command / Right Ctrl | release captured mouse on macOS / other hosts |

Runtime controls are intercepted by the host and are not delivered as guest
PS/2 input.

## Replay Scripts

Scripts are line-oriented deterministic timelines:

```text
at 1 key A down
at 2 key A up
at 3s move 160 -40 during 2s
at 6s mouse 0 0 1
at 6.1s mouse 0 0 0
at 8s rtc 2026-06-16T12:00:00
at 9s caption top 3s REPRODUCIBLE INPUT
at 12s capture out
at 15s capture in
```

Plain timestamps are virtual ticks. `ms` and `s` suffixes are converted at 60
Hz. `move` expands into smooth relative packets. Captions are burned into
captured frames. Capture markers omit quiet sections from videos without
changing the guest timeline; `out` and `in` are accepted short aliases.

```sh
machinelab replay scripts/flappy_caption_demo.mlabscript --headless \
  --video build/flappy.mp4 --video-fps 30 -- build/examples/flappy_bird
```

`--frame-dir` stores numbered PPM frames. `--video` uses ffmpeg to encode MP4.
`--video-fps` controls output rate and down-samples capture work for longer
timelines.

## Paired Runtimes

```sh
machinelab run-pair build/examples/ninjix --right build/examples/ninjix
```

The left and right programs have separate machines, processes, input, display,
and IRQ state. The runtime bridges matching ports: left COM1 to right COM1 and
left COM2 to right COM2. Use `--left-script` and `--right-script` for separate
deterministic input streams.

## Bundles

```sh
machinelab bundle . --program build/examples/flappy_bird --name flappy-bird
./dist/flappy-bird.mlab
```

The default single-file bundle is directly executable. `--format dir` produces
an exploded directory with launchers, manifest, runtime, program, SDK headers,
and static library. Bundling packages existing host binaries; it is not a
cross-compiler.

## Shell Completion

```sh
machinelab completion bash > machinelab.bash
machinelab completion zsh > _machinelab
machinelab completion fish > machinelab.fish
```
