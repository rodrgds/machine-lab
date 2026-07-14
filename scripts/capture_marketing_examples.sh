#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
BUILD_DIR="$ROOT/build"
ASSET_DIR="$ROOT/docs/assets"
DEMO_DIR="$ASSET_DIR/demo"
SCREENSHOT_DIR="$ASSET_DIR/screenshots"
TMP_DIR="$BUILD_DIR/marketing-captures"

if [ ! -f "$BUILD_DIR/CMakeCache.txt" ]; then
  cmake -S "$ROOT" -B "$BUILD_DIR" -DMACHINE_LAB_WITH_SDL=ON
fi

cmake --build "$BUILD_DIR" --target machinelab word_processor music_maker -j4
mkdir -p "$DEMO_DIR" "$SCREENSHOT_DIR" "$TMP_DIR"

rm -f "$DEMO_DIR/word-processor.mp4" "$DEMO_DIR/music-maker.mp4" \
  "$DEMO_DIR/music-maker.wav" "$TMP_DIR/word.ppm" "$TMP_DIR/music.ppm"

"$BUILD_DIR/machinelab" replay "$ROOT/scripts/write_note.mlabscript" \
  --headless --video "$DEMO_DIR/word-processor.mp4" --video-fps 30 \
  --dump-frame "$TMP_DIR/word.ppm" -- "$BUILD_DIR/examples/word_processor"

"$BUILD_DIR/machinelab" replay "$ROOT/scripts/music_maker_demo.mlabscript" \
  --headless --video "$DEMO_DIR/music-maker.mp4" --video-fps 30 \
  --audio-wav "$DEMO_DIR/music-maker.wav" --dump-frame "$TMP_DIR/music.ppm" \
  -- "$BUILD_DIR/examples/music_maker"

ffmpeg -y -loglevel error -i "$TMP_DIR/word.ppm" "$SCREENSHOT_DIR/word-processor.png"
ffmpeg -y -loglevel error -i "$TMP_DIR/music.ppm" "$SCREENSHOT_DIR/music-maker.png"

echo "captured marketing example assets"
