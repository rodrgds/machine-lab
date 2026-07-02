# Examples

Build once before running examples:

```sh
devenv shell
machinelab-build
```

Use the focused examples in the order below when explaining the platform. They
progress from the client/runtime boundary to interrupts, input, mapped memory,
audio, and serial communication.

## Focused Examples

| Program | Demonstrates | Canonical command |
| --- | --- | --- |
| `hello` | initialization, runtime console, process boundary | `machinelab run --headless -- build/examples/hello` |
| `timer_int` | IRQ0 subscription and event waits | `machinelab run --headless -- build/examples/timer_int 3` |
| `rtc_date` | CMOS ports, UIP, BCD conversion, fixed RTC | `machinelab run --headless --rtc 2026-06-16T12:34:56 -- build/examples/rtc_date` |
| `keyboard_scan` | IRQ1 and make/break scancodes | `machinelab run --headless --script scripts/type_a_esc.mlabscript -- build/examples/keyboard_scan` |
| `mouse_packet` | IRQ12, PS/2 enable/ACK, signed packet deltas | `machinelab run --headless --script scripts/mouse_move.mlabscript -- build/examples/mouse_packet` |
| `vbe_rectangle` | VBE mode, framebuffer map, pitch, present | `machinelab run --headless --dump-frame build/rectangle.ppm -- build/examples/vbe_rectangle` |
| `audio_tone` | PCM mapping, synthesis, AC97-lite playback | `machinelab run --headless --audio-wav build/tone.wav -- build/examples/audio_tone` |
| `uart_loopback` | one UART's MCR loopback and RX interrupt | `machinelab run --headless -- build/examples/uart_loopback` |
| `uart_pair` | COM1-to-COM2 cable inside one machine | `machinelab run --headless -- build/examples/uart_pair` |
| `uart_peer_sender` + `receiver` | cable between two runtime processes | `machinelab run-pair --headless build/examples/uart_peer_sender --right build/examples/uart_peer_receiver --max-ticks 800` |
| `sdl_demo` | PIT-driven VBE animation and live keyboard input | `machinelab run build/examples/sdl_demo` |

`uart_pair` and `run-pair` are deliberately different. The former tests two
ports owned by one guest machine; the latter creates two independent machines
and bridges matching ports between them.

## Complete Programs

### Project Seeds

These are small complete apps intended as final-project starting points. Each
one keeps the device-facing pieces visible: IRQ loops, scancode handling,
framebuffer drawing, and device-buffer output.

| Program | Demonstrates | Canonical command |
| --- | --- | --- |
| `word_processor` | cursor editing, wrap/scroll, insert/delete, document stats, VBE UI | `machinelab run --headless --script scripts/write_note.mlabscript --dump-frame build/write.ppm -- build/examples/word_processor` |
| `music_maker` | 3-track tracker, playhead, tempo, mixer bars, AC97 PCM synthesis | `machinelab run --headless --script scripts/music_maker_demo.mlabscript --audio-wav build/music.wav --dump-frame build/music.ppm -- build/examples/music_maker` |
| `breakout_demo` | timer-driven physics, keyboard state, collision, game HUD | `machinelab run --headless --script scripts/breakout_demo.mlabscript --dump-frame build/breakout.ppm -- build/examples/breakout_demo` |

### Flappy Bird

A compact example of timer-driven animation, keyboard input, VBE rendering,
audio, and replay-friendly behavior.

```sh
machinelab run build/examples/flappy_bird
machinelab replay scripts/flappy_caption_demo.mlabscript --headless \
  --video build/flappy.mp4 -- build/examples/flappy_bird
```

### Ninjix

The port of the previous Minix group project. It exercises a larger state
machine, mouse/keyboard input, framebuffer rendering, audio, profiling hooks,
and a UART multiplayer protocol.

```sh
machinelab run build/examples/ninjix
machinelab run-pair build/examples/ninjix --right build/examples/ninjix
machinelab run-pair --headless build/examples/ninjix \
  --right build/examples/ninjix --max-ticks 900
```

In interactive multiplayer, select Multiplayer in both windows and choose
opposite ATTACK and DEFEND roles. Headless paired mode automatically runs a
short role-specific smoke test.

### Plants vs Zombies

A second larger project port that is useful for showing compatibility with a
different codebase and project layout.

```sh
machinelab run build/examples/pvz
machinelab run --headless --script scripts/pvz_menu_exit.mlabscript \
  --dump-frame build/pvz.ppm --max-ticks 240 -- build/examples/pvz
```

## Replay Scripts

| Script | Intended program/use |
| --- | --- |
| `demo_exit.mlabscript` | exit `sdl_demo` after a few ticks |
| `type_a_esc.mlabscript` | keyboard and Lab 3 checks |
| `mouse_move.mlabscript` | one deterministic mouse packet |
| `write_note.mlabscript` | scripted text entry for `word_processor` |
| `music_maker_demo.mlabscript` | edit and exit the tracker grid |
| `breakout_demo.mlabscript` | move the paddle and exit `breakout_demo` |
| `flappy_demo.mlabscript` | short integration replay |
| `flappy_caption_demo.mlabscript` | video/caption/capture-cut showcase |
| `flappy_space_stress.mlabscript` | dense input and frame-capture regression |
| `ninjix_menu_exit.mlabscript` | menu/initialization smoke test |
| `ninjix_level1_demo.mlabscript` | authored long-form Ninjix video timeline |
| `pvz_menu_exit.mlabscript` | PvZ startup and exit smoke test |

Scripts used by automated tests should stay short and stable. Longer authored
video scripts may prioritize presentation timing; verify them visually when
game layouts or balance change.
