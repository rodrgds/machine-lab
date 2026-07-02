# Runtime And CLI

`machinelab` is the supported entry point for running, testing, recording, and
packaging student programs.

## Commands

| Command | Purpose |
| --- | --- |
| `machinelab run` | host one student program |
| `machinelab run-pair` | host two programs with bridged UARTs |
| `machinelab replay` | run one program with an authored input timeline |
| `machinelab setup` | generate a student workspace |
| `machinelab test` | compile and run one predefined lab test |
| `machinelab lab` | list labs or inspect requested functions |
| `machinelab bundle` | package a runtime and program for this platform |
| `machinelab docs cli` | print a Markdown CLI reference |
| `machinelab completion` | emit shell completion |

Use `machinelab <command> --help` for the exact option list.

## Interactive And Headless

```sh
machinelab run build/examples/flappy_bird
machinelab run --headless --max-ticks 300 -- build/examples/timer_int 3
```

Headless runs are deterministic and suitable for tests. SDL runs are for live
teaching, demos, and student exploration.

Useful artifacts:

```sh
machinelab run --headless --trace build/run.jsonl -- build/examples/timer_int 3
machinelab run --headless --dump-frame build/frame.ppm -- build/examples/vbe_rectangle
machinelab run --headless --audio-wav build/audio.wav -- build/examples/audio_tone
```

## Replay Scripts

Replay scripts are line-oriented timelines:

```text
at 1 key A down
at 2 key A up
at 3s move 160 -40 during 2s
at 8s rtc 2026-06-16T12:00:00
at 9s caption top 3s REPRODUCIBLE INPUT
```

They are useful for:

- deterministic demos;
- regression tests;
- project review;
- recorded videos;
- grading artifacts.

## Paired Runtimes

```sh
machinelab run-pair build/examples/ninjix --right build/examples/ninjix
```

The left and right programs have separate machines, processes, input streams,
display state, and IRQ state. The runtime bridges matching serial ports:
left COM1 to right COM1 and left COM2 to right COM2.

This replaces the old two-VM/null-modem setup with one reproducible command.

## Bundles

```sh
machinelab bundle . --program build/examples/flappy_bird --name flappy-bird
./dist/flappy-bird.mlab
```

The default bundle is executable on the current host platform. Bundling packages
existing binaries; it is not cross-compilation.
