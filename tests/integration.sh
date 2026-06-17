#!/bin/sh
set -eu

BIN_DIR="${1:-build}"
EX_DIR="${2:-build/examples}"
OUT_DIR="${3:-build/test-output}"

mkdir -p "$OUT_DIR"

"$BIN_DIR/lcom" lab list > "$OUT_DIR/labs.txt"
grep -q "lab6" "$OUT_DIR/labs.txt"
"$BIN_DIR/lcom" lab show lab5 > "$OUT_DIR/lab5.txt"
grep -q "video_set_mode" "$OUT_DIR/lab5.txt"
"$BIN_DIR/lcom" docs cli > "$OUT_DIR/cli-docs.md"
grep -q "lcom bundle" "$OUT_DIR/cli-docs.md"
"$BIN_DIR/lcom" completion bash > "$OUT_DIR/lcom.bash"
grep -q "_lcom_complete" "$OUT_DIR/lcom.bash"

"$BIN_DIR/lcom" run --headless -- "$EX_DIR/hello" > "$OUT_DIR/hello.txt"
grep -q "hello from liblcom-ng" "$OUT_DIR/hello.txt"

"$BIN_DIR/lcom" run --headless -- "$EX_DIR/timer_int" 3 > "$OUT_DIR/timer.txt"
grep -q "timer tick 3" "$OUT_DIR/timer.txt"

"$BIN_DIR/lcom" run --headless --trace "$OUT_DIR/timer.trace.jsonl" -- "$EX_DIR/timer_int" 1 > "$OUT_DIR/timer_trace.txt"
test -s "$OUT_DIR/timer.trace.jsonl"
grep -q '"event":"irq"' "$OUT_DIR/timer.trace.jsonl"

"$BIN_DIR/lcom" run --headless --script scripts/type_a_esc.lcomscript -- "$EX_DIR/keyboard_scan" > "$OUT_DIR/kbd.txt"
grep -q "kbd make 0x1e" "$OUT_DIR/kbd.txt"
grep -q "kbd break 0x81" "$OUT_DIR/kbd.txt"

"$BIN_DIR/lcom" replay scripts/type_a_esc.lcomscript --headless -- "$EX_DIR/keyboard_scan" > "$OUT_DIR/replay.txt"
grep -q "kbd break 0x81" "$OUT_DIR/replay.txt"

"$BIN_DIR/lcom" run --headless --rtc 2026-06-16T12:34:56 -- "$EX_DIR/rtc_date" > "$OUT_DIR/rtc.txt"
grep -q "rtc 26-06-16 12:34:56" "$OUT_DIR/rtc.txt"

"$BIN_DIR/lcom" run --headless -- "$EX_DIR/uart_loopback" > "$OUT_DIR/uart.txt"
grep -q "uart rx L" "$OUT_DIR/uart.txt"

"$BIN_DIR/lcom" run --headless -- "$EX_DIR/uart_pair" > "$OUT_DIR/uart_pair.txt"
grep -q "uart pair LCOM" "$OUT_DIR/uart_pair.txt"

"$BIN_DIR/lcom" run --headless --audio-wav "$OUT_DIR/tone.wav" -- "$EX_DIR/audio_tone" > "$OUT_DIR/audio.txt"
grep -q "audio tone" "$OUT_DIR/audio.txt"
test -s "$OUT_DIR/tone.wav"
head -c 4 "$OUT_DIR/tone.wav" | grep -q RIFF

"$BIN_DIR/lcom" run --headless --script scripts/mouse_move.lcomscript -- "$EX_DIR/mouse_packet" > "$OUT_DIR/mouse.txt"
grep -q "mouse packet" "$OUT_DIR/mouse.txt"
grep -q "dx=5 dy=-2 lb=1" "$OUT_DIR/mouse.txt"

"$BIN_DIR/lcom" run --headless --dump-frame "$OUT_DIR/rectangle.ppm" -- "$EX_DIR/vbe_rectangle" > "$OUT_DIR/vbe.txt"
grep -q "rectangle drawn" "$OUT_DIR/vbe.txt"
test -s "$OUT_DIR/rectangle.ppm"

"$BIN_DIR/lcom" run --headless --script scripts/demo_exit.lcomscript --dump-frame "$OUT_DIR/sdl_demo.ppm" -- "$EX_DIR/sdl_demo" > "$OUT_DIR/sdl_demo.txt"
grep -q "SDL demo" "$OUT_DIR/sdl_demo.txt"
grep -q "kbd byte 0x81" "$OUT_DIR/sdl_demo.txt"
test -s "$OUT_DIR/sdl_demo.ppm"

"$BIN_DIR/lcom" run --headless --script scripts/flappy_demo.lcomscript --audio-wav "$OUT_DIR/flappy.wav" --dump-frame "$OUT_DIR/flappy.ppm" -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy.txt"
grep -q "LCOM Bird" "$OUT_DIR/flappy.txt"
test -s "$OUT_DIR/flappy.ppm"
test -s "$OUT_DIR/flappy.wav"

"$BIN_DIR/lcom" run --headless --script scripts/flappy_mouse_demo.lcomscript --audio-wav "$OUT_DIR/flappy_mouse.wav" --dump-frame "$OUT_DIR/flappy_mouse.ppm" -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy_mouse.txt"
grep -q "mouse click flaps" "$OUT_DIR/flappy_mouse.txt"
test -s "$OUT_DIR/flappy_mouse.ppm"
test -s "$OUT_DIR/flappy_mouse.wav"

"$BIN_DIR/lcom" run --headless --script scripts/flappy_mouse_stress.lcomscript --dump-frame "$OUT_DIR/flappy_mouse_stress.ppm" -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy_mouse_stress.txt"
grep -q "Program exited with status 0" "$OUT_DIR/flappy_mouse_stress.txt"
test -s "$OUT_DIR/flappy_mouse_stress.ppm"

"$BIN_DIR/lcom" replay scripts/flappy_mouse_demo.lcomscript --headless --video "$OUT_DIR/flappy_replay.mp4" --video-fps 30 -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy_replay.txt"
grep -q "Rendered video" "$OUT_DIR/flappy_replay.txt"
test -s "$OUT_DIR/flappy_replay.mp4"

rm -rf "$OUT_DIR/flappy-headless.lcom"
"$BIN_DIR/lcom" bundle . --program "$EX_DIR/flappy_bird" --name flappy-headless --output "$OUT_DIR/flappy-headless.lcom" --script scripts/flappy_mouse_demo.lcomscript --headless > "$OUT_DIR/bundle.txt"
test -x "$OUT_DIR/flappy-headless.lcom/run.sh"
test -f "$OUT_DIR/flappy-headless.lcom/sdk/include/lcom/lcom.h"
"$OUT_DIR/flappy-headless.lcom/run.sh" > "$OUT_DIR/bundle-run.txt"
grep -q "Program exited with status 0" "$OUT_DIR/bundle-run.txt"

echo "integration tests passed"
