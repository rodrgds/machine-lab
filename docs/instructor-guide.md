# Instructor Guide

This guide is for adopting Machine Lab in a course, workshop, independent study
track, or university lab section. It assumes you want students to learn
machine-facing C ideas without spending the first weeks debugging VM images,
kernel service setup, or platform-specific display/audio tooling.

## Course Promise

Machine Lab gives students:

- C APIs that resemble device-driver work;
- deterministic tests for small lab functions;
- visible artifacts for interactive work: frames, WAVs, traces, videos, bundles;
- final projects that combine devices into a real reactive program.

It gives instructors:

- reusable lab handouts;
- generated student workspaces;
- reference solutions and predefined checks;
- portable release binaries for Linux, macOS, and Windows;
- a project studio path after the labs.

## What To Give Students

- the release binary for their platform, or setup instructions for `machinelab`;
- a generated workspace from `machinelab setup`;
- selected handouts from [course/labs/docs](../course/labs/docs);
- your project brief, collaboration policy, and grading rubric.

Avoid giving students the upstream repository as their primary workspace unless
they are also contributing to Machine Lab itself.

## Syllabus Map

| Week | Student work | Instructor focus | Evidence |
| --- | --- | --- | --- |
| 1 | setup, bit helpers, RTC date/time | workspace smoke test, C bit review | failing then passing `rtc` tests |
| 2 | PIT divisors and timer interrupts | event loops and timing policy | `timer` tests and tick logs |
| 3 | keyboard status and scancodes | polling vs interrupts, partial data | scancode output or replay |
| 4 | mouse packets | packet synchronization and signed deltas | packet test output |
| 5 | VBE mode and rectangles | memory mapping, pitch, clipping | framebuffer dump |
| 6 | sprites and project sketch | image formats, rendering helpers | small visual prototype |
| 7 | PCM audio | buffers, sample rate, generated sound | WAV artifact |
| 8 | UART | FIFOs, loopback, pair runtime | serial trace or two-peer demo |
| 9 | project proposal | scope control and risk review | proposal with devices |
| 10 | first playable/usable loop | state machine and deterministic replay | replay script |
| 11 | systems pass | libraries, error handling, timing | code review |
| 12 | polish and packaging | artifacts, demo script, rubric check | video or bundle |
| 13 | final demo | presentation and review | source plus artifact package |

Shorter modules can skip labs 6 and 7, or run Lab 5 plus a small project as a
graphics-focused unit.

## Lab Track

| Lab | Device idea | Typical check |
| --- | --- | --- |
| 1 | bit masks, CMOS/RTC reads, BCD conversion | `machinelab test rtc` |
| 2 | i8254 PIT divisors and IRQ0 event loops | `machinelab test timer` |
| 3 | i8042 keyboard status and scancodes | `machinelab test kbd` |
| 4 | PS/2 mouse commands and packet parsing | `machinelab test mouse` |
| 5 | VBE modes, mapped framebuffers, sprites | `machinelab test graphics` |
| 6 | AC97-style PCM buffers and playback | `machinelab test audio` |
| 7 | 16550 UART setup, FIFOs, loopback, protocols | `machinelab test uart` |

## Working Methodology

Students should work in small groups or pairs, but each student should be able
to run the workspace locally. Labs are practice for the project; do not let a
single group member become the only person who can run tests.

Suggested rhythm:

1. Assign the handout before lab time.
2. Start the session by running the failing starter test.
3. Discuss the device model and one small bit/register example.
4. Leave implementation gaps for students.
5. End with a reproducible artifact: test output, trace, screenshot, or WAV.

> [!IMPORTANT]
> Resist turning the handouts into full solutions. The useful learning happens
> when students translate a device specification into error handling, byte
> parsing, and event-loop code.

## Project Rubric

Suggested semester weighting for a broad outcome-based rubric:

| Area | Weight | Looks for |
| --- | --- | --- |
| Labs | 35% | passing checks, clean helper boundaries, correct error handling |
| Project prototype | 15% | minimum finished loop, realistic scope, deterministic replay |
| Final project | 35% | device integration, event loop, state management, polish |
| Implementation note | 10% | clear explanation of devices, timing, data flow, tradeoffs |
| Demo artifact | 5% | screenshot, WAV, video, bundle, or trace that can be reproduced |

For an LCOM-style project component, a device-weighted rubric can be more
concrete:

| Area | Weight | Looks for |
| --- | ---: | --- |
| Practical-class individual progress | 10% | each student can run, explain, and advance the work |
| RTC | 5% | purposeful use such as timestamps, calendars, logs, saves, or time-aware behavior |
| Timer | 5% | stable timing policy, animation ticks, debouncing, scheduling, or elapsed-time logic |
| Keyboard | 10% | correct event handling, key state, shortcuts, text input, or command control |
| Mouse | 10% | packet/state handling, coordinates, buttons, dragging, pointing, or gesture logic |
| Graphics card / framebuffer | 20% | correct drawing, clipping, buffering, animation, and visual polish |
| Code structure | 10% | modular C, clear helpers, documented architecture, build hygiene |
| Demonstration and discussion | 20% | convincing live demo and individual technical understanding |
| Report and video | 10% | concise architecture report and reproducible demo artifact |
| Optional serial-port extension | up to 10% extra | real paired feature, protocol, multiplayer, console, or visualizer |

Final projects should also satisfy these engineering criteria:

| Criterion | Strong submission |
| --- | --- |
| Device use | combines at least three device areas meaningfully |
| Event loop | handles time, input, rendering/audio without blocking surprises |
| Architecture | separates device helpers, application state, and presentation |
| Reliability | has deterministic replay or scripted smoke path |
| Finish | has a coherent start, interaction loop, and end state |
| Evidence | includes source, build instructions, and reproducible artifact |

For final delivery, require source, build/run instructions, project
documentation, a report of at most five pages, and a demo video of at most five
minutes. The report should explain the goal, architecture, device usage, and
differentiating features. Treat the assessed version as the last commit before
the hard deadline.

If AI tools are allowed, make responsibility explicit. Students may use tools to
support learning, debugging, planning, and review, but they remain responsible
for every submitted file, must be able to explain the code, must verify
generated output, must avoid confidential data disclosure, and must describe
tool usage if asked.

## Instructor-Owned Surfaces

| Path | Use |
| --- | --- |
| `course/README.md` | course flow and project studio notes |
| `course/labs/docs/` | student-facing handouts |
| `course/labs/function-requests.json` | structured lab index used by CLI/help |
| `examples/` | demos and project seeds |
| `scripts/` | deterministic replays for demos and tests |
| `docs/video-demo.md` | short presentation script |
| `docs/adoption.md` | one-week adoption and public outreach guide |

## Keep Private Or Delayed

| Path | Reason |
| --- | --- |
| `dev/lab-solutions/` | reference implementations |
| `dev/lab-tests/` | useful for grading, but can reveal expected behavior |
| `tests/integration.sh` | upstream regression gate, not student material |

You may publish selected tests if that matches your course policy. Keep
reference implementations private until the course is over.

## Adoption Checklist

- Run `machinelab-test` on one instructor machine.
- Generate a fresh workspace with `machinelab setup student`.
- Confirm `make -C student` works.
- Confirm one TODO lab fails as expected.
- Pick the labs you will assign and hide the rest if the course is shorter.
- Publish a project rubric before students start combining devices.
- Prepare one demo: word processor, music maker, or game replay.
- Decide how students will submit artifacts.

## Assessment Artifacts

Prefer artifacts students can reproduce:

- predefined lab test output;
- short replay scripts;
- screenshots or WAV captures;
- final-project source;
- short implementation notes explaining device use and architecture.

## Running A Demo

Useful commands:

```sh
machinelab run --headless --script scripts/write_note.mlabscript \
  --dump-frame build/write.ppm -- build/examples/word_processor

machinelab run --headless --script scripts/music_maker_demo.mlabscript \
  --audio-wav build/music.wav --dump-frame build/music.ppm -- build/examples/music_maker
```

These examples show that the labs lead somewhere larger than isolated helper
functions: text editing, music sequencing, games, and protocol demos.

## Reuse Policy

Course materials are CC BY 4.0. You can adapt handouts, translate them, change
the schedule, and reuse screenshots or demo media with attribution. Code is MIT
licensed. See [course/LICENSE.md](../course/LICENSE.md) and [LICENSE](../LICENSE).
