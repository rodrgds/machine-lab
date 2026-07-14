#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/../../.." && pwd)"
PUBLIC_DIR="$ROOT/media/remotion/public"

rm -rf "$PUBLIC_DIR"
mkdir -p "$PUBLIC_DIR/screenshots" "$PUBLIC_DIR/clips" "$PUBLIC_DIR/audio"

cp "$ROOT/docs/assets/screenshots/word-processor.png" "$PUBLIC_DIR/screenshots/word-processor.png"
cp "$ROOT/docs/assets/screenshots/music-maker.png" "$PUBLIC_DIR/screenshots/music-maker.png"
cp "$ROOT/docs/assets/screenshots/breakout.png" "$PUBLIC_DIR/screenshots/breakout.png"
cp "$ROOT/docs/assets/screenshots/flappy.png" "$PUBLIC_DIR/screenshots/flappy.png"
cp "$ROOT/docs/assets/screenshots/ninjix.png" "$PUBLIC_DIR/screenshots/ninjix.png"
cp "$ROOT/docs/assets/demo/word-processor.mp4" "$PUBLIC_DIR/clips/word-processor.mp4"
cp "$ROOT/docs/assets/demo/music-maker.mp4" "$PUBLIC_DIR/clips/music-maker.mp4"
cp "$ROOT/docs/assets/demo/music-maker.wav" "$PUBLIC_DIR/audio/music-maker.wav"

echo "synced Remotion assets"
