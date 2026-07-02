#!/usr/bin/env sh
set -eu

test -f package.json
test -f package-lock.json
test -f wrangler.toml
test -f .github/workflows/docs.yml
test -f docs-site/.vitepress/config.mts
test -f docs-site/.vitepress/theme/custom.css
test -f docs-site/index.md
test -f docs-site/students/index.md
test -f docs-site/instructors/index.md
test -f docs-site/instructors/adoption.md
test -f docs-site/developers/index.md
test -f docs-site/developers/architecture.md
test -f docs-site/developers/runtime-cli.md
test -f docs-site/developers/docs-site.md
test -f docs-site/developers/releases.md
test -f docs-site/developers/from-minix.md
test -f docs-site/examples/index.md
test -f docs-site/examples/ninjix.md
test -f docs-site/labs/index.md
test -f media/remotion/package.json
test -f media/remotion/src/Video.tsx
test -f media/remotion/src/Root.tsx
test -f media/remotion/.agents/skills/remotion-best-practices/SKILL.md
test -f media/remotion/.agents/skills/remotion-video-director/SKILL.md
test -f media/remotion/skills-lock.json
test ! -e .agents
test ! -e skills-lock.json

for lab in lab1 lab2 lab3 lab4 lab5; do
  test -f "docs-site/labs/$lab/context.md"
done

for lab in lab1 lab2 lab3 lab4 lab5 lab6 lab7; do
  test -f "docs-site/labs/$lab/index.md"
  test -f "docs-site/labs/$lab/tasks.md"
  test -f "docs-site/labs/$lab/check.md"
  test -f "docs-site/labs/$lab/modern.md"
  grep -q "\\[!IMPORTANT\\]" "docs-site/labs/$lab/tasks.md"
  grep -q "External Reading" "docs-site/labs/$lab/check.md"
  grep -q "How It Works Today" "docs-site/labs/$lab/modern.md"
done

for page in docs-site/labs/lab*/*.md; do
  words=$(wc -w < "$page" | tr -d ' ')
  if [ "$words" -lt 100 ]; then
    echo "lab guide page is too short: $page ($words words)" >&2
    exit 1
  fi

  list_items=$(grep -Ec '^- |^[0-9]+\. ' "$page" || true)
  if [ "$list_items" -gt 14 ]; then
    echo "lab guide page is too list-heavy: $page ($list_items list items)" >&2
    exit 1
  fi
done

test -f docs-site/labs/lab1/io-ports.md
test -f docs-site/labs/lab1/bitwise.md
test -f docs-site/labs/lab1/rtc.md
test -f docs-site/labs/lab2/timer-controller.md
test -f docs-site/labs/lab2/programming.md
test -f docs-site/labs/lab2/interrupts.md
test -f docs-site/labs/lab2/library.md
test -f docs-site/labs/lab3/keyboard.md
test -f docs-site/labs/lab3/scancodes.md
test -f docs-site/labs/lab3/sync.md
test -f docs-site/labs/lab3/kbc-commands.md
test -f docs-site/labs/lab3/multiple-devices.md
test -f docs-site/labs/lab4/mouse-controller.md
test -f docs-site/labs/lab4/packets.md
test -f docs-site/labs/lab5/video-card.md
test -f docs-site/labs/lab5/vbe.md
test -f docs-site/labs/lab5/vram.md
test -f docs-site/labs/lab5/rectangles.md
test -f docs-site/labs/lab5/xpm.md
test -f docs-site/labs/lab5/reactive-apps.md
test -f docs-site/labs/lab6/pcm.md
test -f docs-site/labs/lab7/protocols.md

grep -q "Machine Lab" docs-site/index.md
grep -q "computer architecture" docs-site/index.md
grep -q "Ninjix" docs-site/examples/ninjix.md
grep -q "/assets/screenshots/word-processor.png" docs-site/examples/index.md
grep -q "/assets/screenshots/music-maker.png" docs-site/examples/index.md
grep -q "/assets/screenshots/breakout.png" docs-site/examples/index.md
grep -q "/assets/screenshots/flappy.png" docs-site/examples/index.md
grep -q "/assets/screenshots/ninjix.png" docs-site/examples/index.md
grep -q "/assets/screenshots/ninjix.png" docs-site/examples/ninjix.md
grep -q "runtime/device boundary" docs-site/developers/architecture.md
grep -q "VitePress" docs-site/developers/docs-site.md
grep -q "6 ECTS" docs-site/instructors/adoption.md
grep -q "52 contact hours" docs-site/instructors/adoption.md
grep -q "162 total hours" docs-site/instructors/adoption.md
grep -q "learning by doing" docs-site/instructors/adoption.md
grep -q "40% theory / 60% project" docs-site/instructors/adoption.md
grep -q "Computer Architecture" docs-site/instructors/syllabus.md
grep -q "Operating Systems" docs-site/instructors/syllabus.md
grep -q "deploy_pages" .github/workflows/docs.yml
grep -q "pages_build_output_dir" wrangler.toml
test -d docs-site/.vitepress/dist
test -f docs-site/.vitepress/dist/index.html
test -f docs-site/.vitepress/dist/labs/lab1/index.html
test -f docs-site/.vitepress/dist/labs/lab7/check.html
test -f docs-site/.vitepress/dist/developers/architecture.html
test -f docs-site/.vitepress/dist/examples/ninjix.html
test -f docs-site/public/assets/screenshots/word-processor.png
test -f docs-site/public/assets/screenshots/music-maker.png
test -f docs-site/public/assets/screenshots/breakout.png
test -f docs-site/public/assets/screenshots/flappy.png
test -f docs-site/public/assets/screenshots/ninjix.png
test -f docs-site/.vitepress/dist/assets/screenshots/word-processor.png
test -f docs-site/.vitepress/dist/assets/screenshots/music-maker.png
test -f docs-site/.vitepress/dist/assets/screenshots/breakout.png
test -f docs-site/.vitepress/dist/assets/screenshots/flappy.png
test -f docs-site/.vitepress/dist/assets/screenshots/ninjix.png
test -f docs-site/.vitepress/dist/assets/demo/machine-lab-90s.mp4

echo "docs site checks passed"
