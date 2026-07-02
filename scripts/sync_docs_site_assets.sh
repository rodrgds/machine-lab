#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"
PUBLIC_DIR="$ROOT/docs-site/public/assets"

rm -rf "$PUBLIC_DIR"
mkdir -p "$PUBLIC_DIR/screenshots" "$PUBLIC_DIR/demo"

cp "$ROOT/docs/assets/screenshots/word-processor.png" "$PUBLIC_DIR/screenshots/word-processor.png"
cp "$ROOT/docs/assets/screenshots/music-maker.png" "$PUBLIC_DIR/screenshots/music-maker.png"
cp "$ROOT/docs/assets/screenshots/breakout.png" "$PUBLIC_DIR/screenshots/breakout.png"
cp "$ROOT/docs/assets/screenshots/flappy.png" "$PUBLIC_DIR/screenshots/flappy.png"
cp "$ROOT/docs/assets/screenshots/ninjix.png" "$PUBLIC_DIR/screenshots/ninjix.png"
cp "$ROOT/docs/assets/demo/machine-lab-90s.mp4" "$PUBLIC_DIR/demo/machine-lab-90s.mp4"

echo "synced docs-site assets"
