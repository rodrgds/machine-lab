#!/bin/sh
set -eu

BIN_DIR="${1:-build}"
EX_DIR="${2:-build/examples}"
OUT_DIR="${3:-build/test-output}"
CLI="$BIN_DIR/machinelab"
COMPAT_CLI="$BIN_DIR/lcom"
LOWLAB_CLI="$BIN_DIR/lowlab"

mkdir -p "$OUT_DIR"

test -x "$CLI"
test -x "$COMPAT_CLI"
test -x "$LOWLAB_CLI"
grep -q "# Machine Lab" README.md
grep -q "## Choose Your Lane" README.md
grep -q "Machine Lab developers" README.md
grep -q "MIT" README.md
grep -q "CC BY 4.0" README.md
test -f LICENSE
grep -q "MIT License" LICENSE
test -f course/LICENSE.md
grep -q "CC BY 4.0" course/LICENSE.md
test -f CONTRIBUTING.md
grep -q "Audience Boundaries" CONTRIBUTING.md
test -f docs/index.md
grep -q "Student-facing" docs/index.md
test -f docs/adoption.md
grep -qi "one week" docs/adoption.md
test -f docs-site/index.md
test -f docs-site/.vitepress/config.mts
test -f docs-site/labs/lab1/tasks.md
test -f docs-site/labs/lab7/check.md
grep -q "Machine Lab" docs-site/index.md
grep -q "pages_build_output_dir" wrangler.toml
grep -q "deploy_pages" .github/workflows/docs.yml
test -f docs/student-guide.md
test -f docs/instructor-guide.md
test -f docs/developer-guide.md
grep -q "Usually Ignore These" docs/student-guide.md
grep -q "Audience Boundaries" docs/developer-guide.md
grep -q "Syllabus Map" docs/instructor-guide.md
grep -q "Project Rubric" docs/instructor-guide.md
test -d sdk/include/lcom
test -d sdk/client
test -d course/labs/docs
test -d course/labs/templates
test -d dev/lab-solutions
test -d dev/lab-tests
test ! -e labs
grep -q "# Machine Lab Course" course/README.md
grep -q "Audience Split" course/README.md
grep -q "Final Project Studio" course/README.md
grep -q "guided gaps" course/labs/docs/lab1-bitwise-rtc.md
test -s docs/assets/screenshots/word-processor.png
test -s docs/assets/screenshots/music-maker.png
test -s docs/assets/screenshots/breakout.png
test -s docs/assets/screenshots/flappy.png
test -s docs/assets/demo/machine-lab-demo.mp4
for lab_doc in course/labs/docs/lab*.md; do
  grep -q "\\[!IMPORTANT\\]" "$lab_doc"
  grep -q "\\[!TIP\\]" "$lab_doc"
  grep -q "External Reading" "$lab_doc"
done
"$CLI" --help > "$OUT_DIR/top-help.txt" 2>&1 || true
grep -q "Machine Lab" "$OUT_DIR/top-help.txt"
"$COMPAT_CLI" --help > "$OUT_DIR/lcom-help.txt" 2>&1 || true
grep -q "Machine Lab" "$OUT_DIR/lcom-help.txt"
"$LOWLAB_CLI" --help > "$OUT_DIR/lowlab-help.txt" 2>&1 || true
grep -q "Machine Lab" "$OUT_DIR/lowlab-help.txt"

test -f ".github/workflows/release.yml"
grep -q "ubuntu-24.04" ".github/workflows/release.yml"
grep -q "macos-14" ".github/workflows/release.yml"
grep -q "windows-2025" ".github/workflows/release.yml"
grep -q "machinelab-" ".github/workflows/release.yml"
grep -q "softprops/action-gh-release" ".github/workflows/release.yml"

"$CLI" lab list > "$OUT_DIR/labs.txt"
grep -q "lab7" "$OUT_DIR/labs.txt"
grep -q "lab1 (rtc)" "$OUT_DIR/labs.txt"
"$CLI" lab show lab5 > "$OUT_DIR/lab5.txt"
grep -q "video_set_mode" "$OUT_DIR/lab5.txt"
"$CLI" lab show uart > "$OUT_DIR/lab7.txt"
grep -q "lab7 (uart)" "$OUT_DIR/lab7.txt"
grep -q "uart_config" "$OUT_DIR/lab7.txt"
"$CLI" docs cli > "$OUT_DIR/cli-docs.md"
grep -q "# machinelab CLI" "$OUT_DIR/cli-docs.md"
grep -q "machinelab bundle" "$OUT_DIR/cli-docs.md"
"$CLI" completion bash > "$OUT_DIR/machinelab.bash"
grep -q "_machinelab_complete" "$OUT_DIR/machinelab.bash"

for command in run run-pair replay setup test bundle lab docs completion; do
  "$CLI" "$command" --help > "$OUT_DIR/help-$command.txt" 2>&1
  grep -Fq "$command [OPTIONS]" "$OUT_DIR/help-$command.txt"
  ! grep -q "machinelab: .*requires\|machinelab: unknown" "$OUT_DIR/help-$command.txt"
done

if "$CLI" not-a-command --help > "$OUT_DIR/help-unknown.txt" 2>&1; then
  echo "unknown commands should fail even when followed by --help" >&2
  exit 1
fi
grep -q "unknown command not-a-command" "$OUT_DIR/help-unknown.txt"

if "$CLI" run > "$OUT_DIR/run-missing.txt" 2>&1; then
  echo "machinelab run without a program should fail" >&2
  exit 1
fi
grep -q "run requires <program>" "$OUT_DIR/run-missing.txt"
grep -Fq "run [OPTIONS]" "$OUT_DIR/run-missing.txt"

if "$CLI" bundle > "$OUT_DIR/bundle-missing.txt" 2>&1; then
  echo "machinelab bundle without a project dir should fail" >&2
  exit 1
fi
grep -q "bundle requires <project-dir>" "$OUT_DIR/bundle-missing.txt"
grep -Fq "bundle [OPTIONS] project-dir" "$OUT_DIR/bundle-missing.txt"

if "$CLI" completion > "$OUT_DIR/completion-missing.txt" 2>&1; then
  echo "machinelab completion without a shell should fail" >&2
  exit 1
fi
grep -q "completion requires <shell>" "$OUT_DIR/completion-missing.txt"
grep -Fq "completion [OPTIONS] shell" "$OUT_DIR/completion-missing.txt"

if "$CLI" replay > "$OUT_DIR/replay-missing.txt" 2>&1; then
  echo "machinelab replay without a script/program should fail" >&2
  exit 1
fi
grep -q "replay requires <script>" "$OUT_DIR/replay-missing.txt"
grep -Fq "replay [OPTIONS] script" "$OUT_DIR/replay-missing.txt"
grep -Fq -- "--max-ticks" "$OUT_DIR/replay-missing.txt"

if "$CLI" run-pair > "$OUT_DIR/run-pair-missing.txt" 2>&1; then
  echo "machinelab run-pair without programs should fail" >&2
  exit 1
fi
grep -q "run-pair requires" "$OUT_DIR/run-pair-missing.txt"

"$CLI" run --headless -- "$EX_DIR/hello" > "$OUT_DIR/hello.txt"
grep -q "hello from Machine Lab" "$OUT_DIR/hello.txt"

"$CLI" run --headless -- "$EX_DIR/timer_int" 3 > "$OUT_DIR/timer.txt"
grep -q "timer tick 3" "$OUT_DIR/timer.txt"

"$CLI" run --headless --trace "$OUT_DIR/timer.trace.jsonl" -- "$EX_DIR/timer_int" 1 > "$OUT_DIR/timer_trace.txt"
test -s "$OUT_DIR/timer.trace.jsonl"
grep -q '"event":"irq"' "$OUT_DIR/timer.trace.jsonl"

"$CLI" run --headless --script scripts/type_a_esc.mlabscript -- "$EX_DIR/keyboard_scan" > "$OUT_DIR/kbd.txt"
grep -q "kbd make 0x1e" "$OUT_DIR/kbd.txt"
grep -q "kbd break 0x81" "$OUT_DIR/kbd.txt"

"$CLI" replay scripts/type_a_esc.mlabscript --headless -- "$EX_DIR/keyboard_scan" > "$OUT_DIR/replay.txt"
grep -q "kbd break 0x81" "$OUT_DIR/replay.txt"

"$CLI" run --headless --rtc 2026-06-16T12:34:56 -- "$EX_DIR/rtc_date" > "$OUT_DIR/rtc.txt"
grep -q "rtc 26-06-16 12:34:56" "$OUT_DIR/rtc.txt"

"$CLI" run --headless -- "$EX_DIR/uart_loopback" > "$OUT_DIR/uart.txt"
grep -q "uart rx L" "$OUT_DIR/uart.txt"

"$CLI" run --headless -- "$EX_DIR/uart_pair" > "$OUT_DIR/uart_pair.txt"
grep -q "uart pair LCOM" "$OUT_DIR/uart_pair.txt"

"$CLI" run-pair --headless "$EX_DIR/uart_peer_sender" --right "$EX_DIR/uart_peer_receiver" --max-ticks 5000 > "$OUT_DIR/run_pair_uart.txt" 2>&1
grep -q "peer receiver got PAIR" "$OUT_DIR/run_pair_uart.txt"
grep -q "\\[left\\] Program exited with status 0" "$OUT_DIR/run_pair_uart.txt"
grep -q "\\[right\\] Program exited with status 0" "$OUT_DIR/run_pair_uart.txt"
! grep -q "max virtual ticks reached" "$OUT_DIR/run_pair_uart.txt"

if "$CLI" run-pair --headless /usr/bin/false --right /usr/bin/false \
    > "$OUT_DIR/run_pair_failure.txt" 2>&1; then
  echo "run-pair must propagate guest failures" >&2
  exit 1
fi
grep -q "\\[left\\] Program exited with status 1" "$OUT_DIR/run_pair_failure.txt"
grep -q "\\[right\\] Program exited with status 1" "$OUT_DIR/run_pair_failure.txt"

"$CLI" run-pair --headless "$EX_DIR/ninjix" --right "$EX_DIR/ninjix" --max-ticks 900 > "$OUT_DIR/run_pair_ninjix.txt"
grep -q "ninjix pair started as ATTACK" "$OUT_DIR/run_pair_ninjix.txt"
grep -q "ninjix pair started as DEFEND" "$OUT_DIR/run_pair_ninjix.txt"
grep -q "ninjix pair smoke complete" "$OUT_DIR/run_pair_ninjix.txt"
! grep -q "max virtual ticks reached" "$OUT_DIR/run_pair_ninjix.txt"

rm -rf "$OUT_DIR/student_workspace"
"$CLI" setup "$OUT_DIR/student_workspace" > "$OUT_DIR/setup.txt"
test -f "$OUT_DIR/student_workspace/include/lcom/lcom.h"
test -f "$OUT_DIR/student_workspace/Makefile"
test -f "$OUT_DIR/student_workspace/proj/main.c"
test -f "$OUT_DIR/student_workspace/labs/rtc/bitwise.c"
test -f "$OUT_DIR/student_workspace/labs/uart/include/uart_lib.h"
test -f "$OUT_DIR/student_workspace/lib/kbc/kbc.h"
grep -q "## Edit These" "$OUT_DIR/student_workspace/README.md"
grep -q "## Usually Ignore These" "$OUT_DIR/student_workspace/README.md"
grep -q "Machine Lab developer internals" "$OUT_DIR/student_workspace/README.md"
make -C "$OUT_DIR/student_workspace" labs > "$OUT_DIR/student_make_labs.txt"
if "$CLI" test rtc --project "$OUT_DIR/student_workspace" > "$OUT_DIR/student_rtc.txt" 2>&1; then
  echo "fresh student rtc TODO stubs should fail predefined tests" >&2
  exit 1
fi
grep -q "Running rtc predefined tests" "$OUT_DIR/student_rtc.txt"
grep -q "check failed" "$OUT_DIR/student_rtc.txt"

"$CLI" run --headless --audio-wav "$OUT_DIR/tone.wav" -- "$EX_DIR/audio_tone" > "$OUT_DIR/audio.txt"
grep -q "audio tone" "$OUT_DIR/audio.txt"
test -s "$OUT_DIR/tone.wav"
head -c 4 "$OUT_DIR/tone.wav" | grep -q RIFF

if "$CLI" run --headless --audio-wav "$OUT_DIR" -- "$EX_DIR/audio_tone" \
    > "$OUT_DIR/audio_write_failure.txt" 2>&1; then
  echo "audio output failures must fail the run" >&2
  exit 1
fi

if "$CLI" run --headless --dump-frame "$OUT_DIR" -- "$EX_DIR/vbe_rectangle" \
    > "$OUT_DIR/frame_write_failure.txt" 2>&1; then
  echo "frame output failures must fail the run" >&2
  exit 1
fi

"$CLI" run --headless --script scripts/write_note.mlabscript --dump-frame "$OUT_DIR/word_processor.ppm" -- "$EX_DIR/word_processor" > "$OUT_DIR/word_processor.txt"
grep -q "word processor text Machine Lab notes" "$OUT_DIR/word_processor.txt"
grep -q "Portable C, visible results." "$OUT_DIR/word_processor.txt"
grep -q "Tests, traces, screenshots, sound." "$OUT_DIR/word_processor.txt"
test -s "$OUT_DIR/word_processor.ppm"

"$CLI" run --headless --script scripts/music_maker_demo.mlabscript --audio-wav "$OUT_DIR/music_maker.wav" --dump-frame "$OUT_DIR/music_maker.ppm" -- "$EX_DIR/music_maker" > "$OUT_DIR/music_maker.txt"
grep -q "music maker pattern tracks=4 steps=16 active=" "$OUT_DIR/music_maker.txt"
grep -Eq "submitted=([2-9]|[1-9][0-9]+)" "$OUT_DIR/music_maker.txt"
test -s "$OUT_DIR/music_maker.wav"
head -c 4 "$OUT_DIR/music_maker.wav" | grep -q RIFF
music_duration=$(ffprobe -v error -show_entries format=duration -of default=noprint_wrappers=1:nokey=1 "$OUT_DIR/music_maker.wav")
awk -v duration="$music_duration" 'BEGIN { exit !(duration > 1.5) }'
test -s "$OUT_DIR/music_maker.ppm"

"$CLI" run --headless --script scripts/breakout_demo.mlabscript --dump-frame "$OUT_DIR/breakout_demo.ppm" -- "$EX_DIR/breakout_demo" > "$OUT_DIR/breakout_demo.txt"
grep -q "breakout demo score=" "$OUT_DIR/breakout_demo.txt"
test -s "$OUT_DIR/breakout_demo.ppm"

if "$CLI" run --headless --max-ticks 500 -- "$EX_DIR/word_processor" > "$OUT_DIR/word_processor_lifetime.txt" 2>&1; then
  echo "word_processor should keep running until ESC" >&2
  exit 1
fi
grep -q "max virtual ticks reached" "$OUT_DIR/word_processor_lifetime.txt"

if "$CLI" run --headless --max-ticks 500 -- "$EX_DIR/music_maker" > "$OUT_DIR/music_maker_lifetime.txt" 2>&1; then
  echo "music_maker should keep running until ESC" >&2
  exit 1
fi
grep -q "max virtual ticks reached" "$OUT_DIR/music_maker_lifetime.txt"

if "$CLI" run --headless --max-ticks 500 -- "$EX_DIR/breakout_demo" > "$OUT_DIR/breakout_demo_lifetime.txt" 2>&1; then
  echo "breakout_demo should keep running until ESC" >&2
  exit 1
fi
grep -q "max virtual ticks reached" "$OUT_DIR/breakout_demo_lifetime.txt"

"$CLI" run --headless --script scripts/mouse_move.mlabscript -- "$EX_DIR/mouse_packet" > "$OUT_DIR/mouse.txt"
grep -q "mouse packet" "$OUT_DIR/mouse.txt"
grep -q "dx=5 dy=-2 lb=1" "$OUT_DIR/mouse.txt"

"$CLI" run --headless --dump-frame "$OUT_DIR/rectangle.ppm" -- "$EX_DIR/vbe_rectangle" > "$OUT_DIR/vbe.txt"
grep -q "rectangle drawn" "$OUT_DIR/vbe.txt"
test -s "$OUT_DIR/rectangle.ppm"

"$CLI" run --headless --script scripts/demo_exit.mlabscript --dump-frame "$OUT_DIR/sdl_demo.ppm" -- "$EX_DIR/sdl_demo" > "$OUT_DIR/sdl_demo.txt"
grep -q "SDL demo" "$OUT_DIR/sdl_demo.txt"
grep -q "kbd byte 0x81" "$OUT_DIR/sdl_demo.txt"
test -s "$OUT_DIR/sdl_demo.ppm"

"$CLI" run --headless --script scripts/flappy_demo.mlabscript --audio-wav "$OUT_DIR/flappy.wav" --dump-frame "$OUT_DIR/flappy.ppm" -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy.txt"
grep -q "LCOM Bird" "$OUT_DIR/flappy.txt"
test -s "$OUT_DIR/flappy.ppm"
test -s "$OUT_DIR/flappy.wav"

rm -rf "$OUT_DIR/flappy_space_frames"
"$CLI" run --headless --script scripts/flappy_space_stress.mlabscript --audio null --frame-dir "$OUT_DIR/flappy_space_frames" -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy_space_stress.txt"
grep -q "Program exited with status 0" "$OUT_DIR/flappy_space_stress.txt"
test "$(find "$OUT_DIR/flappy_space_frames" -name 'frame_*.ppm' | wc -l | tr -d ' ')" -ge 45

"$CLI" run --headless --script scripts/ninjix_menu_exit.mlabscript --dump-frame "$OUT_DIR/ninjix_menu.ppm" -- "$EX_DIR/ninjix" > "$OUT_DIR/ninjix_menu.txt"
grep -q "Program exited with status 0" "$OUT_DIR/ninjix_menu.txt"
! grep -q "serial .* failed" "$OUT_DIR/ninjix_menu.txt"
test -s "$OUT_DIR/ninjix_menu.ppm"

"$CLI" run --headless --script scripts/pvz_menu_exit.mlabscript --dump-frame "$OUT_DIR/pvz_menu.ppm" --max-ticks 240 -- "$EX_DIR/pvz" > "$OUT_DIR/pvz_menu.txt"
grep -q "Program exited with status 0" "$OUT_DIR/pvz_menu.txt"
test -s "$OUT_DIR/pvz_menu.ppm"

"$CLI" replay scripts/flappy_demo.mlabscript --headless --video "$OUT_DIR/flappy_replay.mp4" --video-fps 30 -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy_replay.txt"
grep -q "Rendered video" "$OUT_DIR/flappy_replay.txt"
test -s "$OUT_DIR/flappy_replay.mp4"

"$CLI" replay scripts/flappy_caption_demo.mlabscript --headless --video "$OUT_DIR/flappy_caption.mp4" --video-fps 30 -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy_caption.txt"
grep -q "Rendered video" "$OUT_DIR/flappy_caption.txt"
test -s "$OUT_DIR/flappy_caption.mp4"

if [ "${LCOM_TEST_SDL:-0}" = "1" ]; then
  "$CLI" replay scripts/flappy_caption_demo.mlabscript --audio null --dump-frame "$OUT_DIR/flappy_replay_sdl.ppm" -- "$EX_DIR/flappy_bird" > "$OUT_DIR/flappy_replay_sdl.txt"
  grep -q "Program exited with status 0" "$OUT_DIR/flappy_replay_sdl.txt"
  test -s "$OUT_DIR/flappy_replay_sdl.ppm"
fi

rm -rf "$OUT_DIR/flappy-headless.mlab"
"$CLI" bundle . --program "$EX_DIR/flappy_bird" --name flappy-headless --output "$OUT_DIR/flappy-headless.mlab" --script scripts/flappy_demo.mlabscript --headless > "$OUT_DIR/bundle.txt"
test -x "$OUT_DIR/flappy-headless.mlab"
"$OUT_DIR/flappy-headless.mlab" > "$OUT_DIR/bundle-run.txt"
grep -q "Program exited with status 0" "$OUT_DIR/bundle-run.txt"

rm -rf "$OUT_DIR/flappy-headless-dir.mlab"
"$CLI" bundle . --format dir --program "$EX_DIR/flappy_bird" --name flappy-headless-dir --output "$OUT_DIR/flappy-headless-dir.mlab" --script scripts/flappy_demo.mlabscript --headless > "$OUT_DIR/bundle-dir.txt"
test -x "$OUT_DIR/flappy-headless-dir.mlab/run.sh"
test -f "$OUT_DIR/flappy-headless-dir.mlab/sdk/include/lcom/lcom.h"

echo "integration tests passed"
