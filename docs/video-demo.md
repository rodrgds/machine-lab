# Teacher Video Demo

This walkthrough is ordered to tell one story: normal tooling, preserved device
learning, deterministic testing, a complete project, and a student workspace.
Run it once before recording so Nix, the build, and ffmpeg are warm.

For a short public-facing version, use
[docs/assets/demo/machine-lab-90s.mp4](assets/demo/machine-lab-90s.mp4). It can
be regenerated with:

```sh
devenv shell -- sh scripts/render_public_demo.sh
```

## Prepare

From the repository root:

```sh
devenv shell
machinelab-test
rm -rf student build/video-demo
mkdir -p build/video-demo
```

Expected result: all nine CTest entries pass. Keep the terminal at a readable
width so commands and test names remain visible.

## 1. Establish the Model

Show these three paths briefly:

```sh
ls sdk/include/lcom runtime/devices examples
```

Explain that a student program includes the public C headers, while `machinelab run`
hosts virtual devices. SDL is only a host backend; student code does not call
SDL. Open `examples/timer_int.c` as the smallest interrupt-driven illustration.

## 2. Run Focused Device Examples

```sh
machinelab run --headless -- build/examples/timer_int 3
machinelab run --headless --rtc 2026-06-16T12:34:56 -- build/examples/rtc_date
machinelab run --headless --script scripts/type_a_esc.mlabscript -- \
  build/examples/keyboard_scan
machinelab run --headless --script scripts/mouse_move.mlabscript -- \
  build/examples/mouse_packet
```

These demonstrate IRQ events, deterministic RTC state, keyboard scancodes, and
raw PS/2 packets without relying on live input.

Produce inspectable graphics and audio artifacts:

```sh
machinelab run --headless --dump-frame build/video-demo/rectangle.ppm -- \
  build/examples/vbe_rectangle
machinelab run --headless --audio-wav build/video-demo/tone.wav -- \
  build/examples/audio_tone
```

## 3. Show Interactive Runtime Tooling

```sh
machinelab run build/examples/sdl_demo
```

In the window:

1. Press F3 to show the debug overlay.
2. Point out scancode/mouse history and runtime metrics.
3. Press F11 if fullscreen helps the recording.
4. Press Escape to exit.

On macOS, Command releases the captured mouse. On other hosts, use Right Ctrl.

## 4. Show a Complete Existing Project

The old Minix group project was ported as Ninjix. Run one client:

```sh
machinelab run build/examples/ninjix
```

Then show that the runtime can replace a two-VM/null-modem setup with one
command:

```sh
machinelab run-pair build/examples/ninjix --right build/examples/ninjix
```

Choose Multiplayer in both windows, then choose opposite ATTACK and DEFEND
roles. COM1 is bridged to COM1 and COM2 to COM2 across the two hosted programs.

For a fast deterministic proof of the same bridge:

```sh
machinelab run-pair --headless build/examples/uart_peer_sender \
  --right build/examples/uart_peer_receiver --max-ticks 800
```

## 5. Show Project Seeds

These are smaller than Ninjix but useful as final-project prompts. They are
interactive apps, so the scripts below only provide deterministic demo exits:

```sh
machinelab run --headless --script scripts/write_note.mlabscript \
  --dump-frame build/video-demo/write.ppm -- build/examples/word_processor
machinelab run --headless --script scripts/music_maker_demo.mlabscript \
  --audio-wav build/video-demo/music.wav --dump-frame build/video-demo/music.ppm -- \
  build/examples/music_maker
machinelab run --headless --script scripts/breakout_demo.mlabscript \
  --dump-frame build/video-demo/breakout.ppm -- build/examples/breakout_demo
```

## 6. Render a Reproducible Demo Video

```sh
machinelab replay scripts/flappy_caption_demo.mlabscript --headless \
  --video build/video-demo/flappy.mp4 --video-fps 30 -- \
  build/examples/flappy_bird
```

This replay demonstrates authored input, captions, and capture pause/resume
markers. The MP4 is deterministic and can be used as B-roll or grading proof.

## 7. Show the Student Experience

```sh
machinelab setup student
make -C student
machinelab lab list
machinelab lab show timer
machinelab test timer --project student
```

The final command should fail because the generated implementation contains
TODOs. Open `student/labs/timer/timer_lab.c`, then compare its small public
contract with the old LCF/Minix ceremony described in
[From Minix to Machine Lab](from-minix.md).

Explain that the predefined test is visible under
`student/.mlab/tests/labs/lab2_test.c`; the workflow is deterministic rather
than dependent on manual VM input.

## 8. Close With Portability

```sh
machinelab bundle . --program build/examples/flappy_bird --name flappy-bird
./dist/flappy-bird.mlab
```

The bundle packages the runtime and program for the current host platform. It
does not cross-compile; a bundle for another OS still needs matching binaries.

## Short Version

If the recording must stay under five minutes, use only:

```sh
machinelab-test
machinelab run build/examples/sdl_demo
machinelab run-pair --headless build/examples/uart_peer_sender \
  --right build/examples/uart_peer_receiver --max-ticks 800
machinelab replay scripts/flappy_caption_demo.mlabscript --headless \
  --video build/video-demo/flappy.mp4 -- build/examples/flappy_bird
machinelab setup student
machinelab test timer --project student
```
