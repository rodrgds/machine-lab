# Machine Lab Course

These labs teach machine-facing C ideas through device-shaped exercises. They are
inspired by older PC/Minix lab work, but the student workflow is portable:
write C, run `machinelab`, inspect deterministic artifacts, iterate.

## Audience Split

| Audience | Primary docs | Edit area |
| --- | --- | --- |
| Students | [student guide](../docs/student-guide.md), lab handouts below | generated workspace |
| Instructors | [instructor guide](../docs/instructor-guide.md), this page | lab sequence and project brief |
| Machine Lab developers | [developer guide](../docs/developer-guide.md) | upstream repo |

The handouts are intentionally incomplete. They introduce the controller model,
show the first pieces of reasoning, and leave guided gaps that students must
fill in their own code.

## Lab Track

| order | folder | theme | check |
| --- | --- | --- | --- |
| `lab1` | `rtc/` | bit masks, bytes, CMOS/RTC reads | `machinelab test rtc` |
| `lab2` | `timer/` | i8254 PIT programming and IRQ0 | `machinelab test timer` |
| `lab3` | `kbd/` | i8042 keyboard and scancodes | `machinelab test kbd` |
| `lab4` | `mouse/` | PS/2 mouse commands and packets | `machinelab test mouse` |
| `lab5` | `graphics/` | VBE modes, framebuffer, XPM sprites | `machinelab test graphics` |
| `lab6` | `audio/` | AC97-lite PCM buffers and playback | `machinelab test audio` |
| `lab7` | `uart/` | 16550 UARTs and serial protocols | `machinelab test uart` |

## Method

- Read the short device model before touching code.
- Build the starter workspace and run the failing test once.
- Implement only the requested functions first.
- Move reusable helpers into `lib/<device>/` after the tested entry points work.
- Keep small notes on what each register bit means; later labs reuse the habit.
- Treat generated traces, frames, WAVs, and replays as evidence, not decoration.

## Student Workspace

Generate a workspace with:

```sh
machinelab setup student
```

The generated project contains:

- `include/lcom/`: public runtime/device headers.
- `labs/<device>/include/`: the tested API for each lab.
- `labs/<device>/*_lab.c`: starter implementations with guided TODOs.
- `lib/<device>/`: optional helper headers students can extend.
- `proj/`: a tiny app linked with the lab objects.
- `Makefile`: a plain build path, no CMake required for students.

Typical loop:

```sh
cd student
make
machinelab test rtc
machinelab test uart
```

Fresh TODO stubs should compile and fail their predefined checks.

## Handouts

- [Lab 1: Bitwise Helpers And RTC](labs/docs/lab1-bitwise-rtc.md)
- [Lab 2: PIT And Timer IRQs](labs/docs/lab2-timer.md)
- [Lab 3: Keyboard And i8042](labs/docs/lab3-keyboard.md)
- [Lab 4: PS/2 Mouse Packets](labs/docs/lab4-mouse.md)
- [Lab 5: VBE Framebuffer And XPM](labs/docs/lab5-video.md)
- [Lab 6: AC97-lite PCM Audio](labs/docs/lab6-audio.md)
- [Lab 7: UART And Serial Ports](labs/docs/lab7-uart.md)

## Final Project Studio

After the labs, students should build a reactive C application that uses at
least three device areas. Good projects are small enough to finish and rich
enough to need real architecture:

- an arcade game with keyboard, timer, framebuffer, and audio;
- a two-player serial game using `run-pair`;
- a drawing or sprite tool with mouse input and exported frames;
- a tiny protocol demo with UART retry/acknowledgement logic;
- a visualization that combines RTC, audio, and animation.

Suggested project checkpoints:

- Proposal: controls, devices, risks, and minimum finished loop.
- Prototype: one screen, one input path, one deterministic replay.
- Systems pass: reusable device helpers, state machine, timing policy.
- Polish pass: capture artifacts, short demo script, packaged bundle.
- Final review: source, replay/video evidence, and written implementation notes.

## Instructor Notes

- `course/labs/function-requests.json` is the structured lab index.
- `course/labs/templates/labN/include/` contains contracts copied by `machinelab setup`.
- `dev/lab-solutions/labN/` contains reference implementations for tests/review.
- `dev/lab-tests/` contains predefined checks copied into generated workspaces.
- The C API still uses `lcom_*` names for compatibility with existing examples;
  the public product and CLI are Machine Lab / `machinelab`.
- Reuse terms for handouts and course docs are in [LICENSE.md](LICENSE.md).

## Public Adoption

Use [docs/adoption.md](../docs/adoption.md) when preparing a new class or
workshop. It includes a one-week rollout checklist, first-session flow, suggested
announcement, and initial public-marketing notes.
