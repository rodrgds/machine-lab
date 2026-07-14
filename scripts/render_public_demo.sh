#!/usr/bin/env sh
set -eu

ROOT="$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)"

"$ROOT/scripts/capture_marketing_examples.sh"

cd "$ROOT/media/remotion"
npm run render
