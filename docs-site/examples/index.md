# Examples

Examples show where the labs lead: small focused programs, project seeds, and
larger ports.

## Focused Examples

| Program | Demonstrates |
| --- | --- |
| `hello` | initialization, runtime console, process boundary |
| `timer_int` | IRQ0 subscription and event waits |
| `rtc_date` | CMOS ports, UIP, BCD conversion, fixed RTC |
| `keyboard_scan` | IRQ1 and make/break scancodes |
| `mouse_packet` | IRQ12, PS/2 enable/ACK, signed packet deltas |
| `vbe_rectangle` | VBE mode, framebuffer map, pitch, present |
| `audio_tone` | PCM mapping, synthesis, AC97-lite playback |
| `uart_loopback` | UART loopback and RX interrupt |
| `uart_peer_sender` + `receiver` | two hosted machines bridged by serial |

## Project Seeds

| Program | Demonstrates |
| --- | --- |
| `word_processor` | editor loop, cursor state, text wrapping, framebuffer UI |
| `music_maker` | tracker grid, tempo, playhead, generated PCM |
| `breakout_demo` | timer physics, keyboard state, collision, HUD |
| `flappy_bird` | compact game loop, audio, replay, capture |
| [Ninjix](/examples/ninjix) | larger port with input, graphics, audio, profiling, UART multiplayer |

## Screenshots

These are real captured frames from the included examples. They are meant to
show the range of final-project directions: tools, trackers, arcade games, and
larger ports.

<div class="screenshot-grid">
  <figure>
    <img src="/assets/screenshots/word-processor.png" alt="Word processor example showing a document canvas with typed text and editor status bars">
    <figcaption><code>word_processor</code>: text editing, cursor state, wrapping, and framebuffer UI.</figcaption>
  </figure>
  <figure>
    <img src="/assets/screenshots/music-maker.png" alt="Music maker tracker example showing tracks, note grid, play button, and mixer meters">
    <figcaption><code>music_maker</code>: tracker grid, playhead, tempo, instruments, and PCM output.</figcaption>
  </figure>
  <figure>
    <img src="/assets/screenshots/breakout.png" alt="Breakout demo showing paddle, ball, bricks, score, and lives">
    <figcaption><code>breakout_demo</code>: timer physics, keyboard state, collision, and HUD.</figcaption>
  </figure>
  <figure>
    <img src="/assets/screenshots/flappy.png" alt="Flappy Bird style demo showing side-scrolling pipes and player character">
    <figcaption><code>flappy_bird</code>: compact game loop, replay capture, and audio hooks.</figcaption>
  </figure>
  <figure>
    <img src="/assets/screenshots/ninjix.png" alt="Ninjix menu showing play, multiplayer, instructions, credits, and exit options">
    <figcaption><a href="/examples/ninjix">Ninjix</a>: larger port with menus, input, graphics, audio hooks, and UART multiplayer.</figcaption>
  </figure>
</div>

## Canonical Commands

```sh
machinelab run --headless --script scripts/write_note.mlabscript \
  --dump-frame build/write.ppm -- build/examples/word_processor

machinelab run --headless --script scripts/music_maker_demo.mlabscript \
  --audio-wav build/music.wav --dump-frame build/music.ppm -- build/examples/music_maker

machinelab run-pair --headless build/examples/uart_peer_sender \
  --right build/examples/uart_peer_receiver --max-ticks 800
```
