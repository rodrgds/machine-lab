# lcom-ng

`lcom-ng` is a userspace LCOM machine runtime. Student programs are normal C
processes linked with `liblcom-ng`; `lcom run` hosts the virtual devices they
talk to through ports, IRQ subscriptions, virtual physical memory, and VBE mode
calls.

The current implementation includes:

- `liblcom-ng` C API: init/exit, console output, port I/O, IRQ events, VBE mode
  info, virtual physical memory mapping, VBE present, and AC97-lite PCM buffer
  mapping/playback.
- `lcom run`: process boundary, stdout/stderr capture, headless deterministic
  virtual time, script injection, framebuffer PPM dump, JSONL traces, and
  optional SDL backend selection.
- `lcom replay`: deterministic script replay with optional frame-sequence and
  MP4 rendering through ffmpeg.
- `lcom bundle`: create a runnable app directory containing the runtime,
  student binary, optional replay script, SDK headers, static lib, manifest, and
  launcher scripts.
- CLI11-backed command registry for help/docs plus `lcom completion` scripts for
  bash, zsh, and fish.
- Virtual devices: i8254 PIT, i8042 keyboard/mouse path, RTC/CMOS, VBE
  framebuffer, AC97-lite audio, and paired 16550-style COM1/COM2 UARTs with
  explicit local loopback.
- Examples for console, timer interrupts, keyboard scancode scanning, RTC, VBE
  rectangles, mouse packets, audio, UART, SDL, and Flappy Bird.
- Unit, lab solution, and integration tests.

## Build And Test

Use the dev shell when you want SDL support:

```sh
devenv shell
lcom-test
```

Or build without entering the shell:

```sh
devenv shell -- lcom-test
```

Plain CMake also works when the dependencies are already installed:

```sh
cmake -S . -B build -DLCOM_WITH_SDL=OFF
cmake --build build
ctest --test-dir build --output-on-failure
```

Run an example:

```sh
build/lcom run build/examples/flappy_bird
build/lcom run build/examples/sdl_demo
build/lcom run --headless -- build/examples/timer_int 3
build/lcom run --headless --script scripts/type_a_esc.lcomscript -- build/examples/keyboard_scan
build/lcom run --headless --script scripts/mouse_move.lcomscript -- build/examples/mouse_packet
build/lcom run --headless --dump-frame build/rectangle.ppm -- build/examples/vbe_rectangle
build/lcom run --headless --audio-wav build/tone.wav -- build/examples/audio_tone
build/lcom run --headless -- build/examples/uart_pair
build/lcom replay scripts/flappy_mouse_demo.lcomscript --headless --video build/flappy.mp4 -- build/examples/flappy_bird
build/lcom bundle . --program build/examples/flappy_bird --name flappy-bird
build/lcom completion zsh > _lcom
build/lcom run --display sdl -- build/examples/sdl_demo
build/lcom run --display sdl build/examples/flappy_bird
```

## Script Format

The script format is line-based:

```text
at 1 key A down
at 2 key A up
at 3 key ESC down
at 4 key ESC up
at 10 mouse 5 -2 1
at 20 rtc 2026-06-16T12:00:00
```

Ticks are virtual and deterministic in headless mode.

`lcom replay <script>` accepts the same runtime options as `lcom run`, including
`--frame-dir <dir>` for numbered PPM frames and `--video <path>` for an encoded
MP4 when ffmpeg is available.

## Labs

Student-facing lab material lives under `labs/`. Starter headers are in
`labs/templates/<lab>/include/`, complete reference solutions are in
`labs/solutions/<lab>/`, and CTest runs `lab1_solution` through `lab6_solution`
against those solutions as normal `lcom run` programs.

## Student API Sketch

```c
#include <lcom/lcom.h>
#include <lcom/i8254.h>

int main(void) {
  lcom_init();

  lcom_irq_t timer;
  lcom_irq_subscribe(TIMER0_IRQ, 0, &timer);

  lcom_event_t ev;
  lcom_event_wait(&ev);
  if (ev.irq_mask & timer.mask) {
    lcom_printf("timer fired\n");
  }

  lcom_exit();
}
```

## SDL Backend

The architecture keeps SDL3 as a host backend, not as a student API. The checked
in SDL3 backend is optional; the default plain CMake build can stay headless
when SDL3 development packages are not installed.

SDL display runs enable realtime virtual ticks by default, so this works without
an explicit `--realtime`:

```sh
build/lcom run build/examples/flappy_bird
```

Pass `--headless` for deterministic test/CI runs.

With the dev shell:

```sh
devenv shell
lcom-run-sdl
```

SDL remains a runtime host backend. Student code still talks to virtual PS/2,
VBE, PIT, RTC, and UART devices through the `lcom-ng` C API.
