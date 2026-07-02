# Syllabus Map

This page gives instructors a concrete semester shape. It is not a required
contract; it is a starting point that mirrors the structure of a practical
computer-laboratory course: device labs first, project studio second.

## Course Summary

Machine Lab teaches students to write structured C code that interacts with
machine-like device interfaces. Students learn bit-level protocols, polling,
interrupt-style events, mapped buffers, byte streams, systematic debugging, and
state-machine design. The practical outcome is a final interactive program that
combines several device areas.

A full course can be framed as a 6 ECTS offering with about 52 contact hours and
162 total hours. A typical weekly format is two hours of theory or guided
discussion plus two hours of laboratory work. The remaining time is independent
study, implementation, debugging, and project work.

## Prerequisites

Students should have passed, or be concurrently prepared by, courses with the
content normally covered in introductory Programming, Computer Architecture, and
Operating Systems. They should be able to compile C programs, read pointer-based
APIs, understand basic memory layout, and reason about registers, interrupts,
and system calls at a conceptual level.

## Learning Outcomes

By the end of the course, successful students should be able to use
device-shaped programmatic interfaces, structure low-level C libraries, debug
with reproducible experiments, combine asynchronous input sources, and build a
reactive application with a clear state model.

The course also develops tool fluency: compiler diagnostics, Makefiles, static
libraries, Git, generated tests, frame/audio artifacts, and short technical
notes that explain implementation decisions.

## Weekly Map

| Week | Student work | Instructor focus | Evidence |
| --- | --- | --- | --- |
| 1 | setup, bit helpers, RTC date/time | workspace smoke test, C bit review | failing then passing `rtc` checks |
| 2 | PIT divisors and timer interrupts | event loops and timing policy | `timer` tests and tick logs |
| 3 | keyboard status and scancodes | polling vs interrupts, partial data | scancode output or replay |
| 4 | mouse packets | synchronization and signed deltas | packet output |
| 5 | graphics mode and rectangles | memory mapping, pitch, clipping | framebuffer dump |
| 6 | sprites and project sketch | rendering helpers, project scope | visual prototype |
| 7 | PCM audio | buffers, sample rate, generated sound | WAV artifact |
| 8 | UART | FIFOs, loopback, pair runtime | serial trace or two-peer demo |
| 9 | project proposal | scope control and risk review | idea, devices, smallest loop, risks |
| 10 | first usable loop | state machine and deterministic replay | replay script |
| 11 | systems pass | helper boundaries, error handling, timing | code review |
| 12 | polish and packaging | artifacts, demo script, rubric check | source, docs, report draft, video draft |
| 13 | final demo | presentation and review | last assessed commit plus artifacts |

Shorter modules can skip Labs 6 and 7, or use Lab 5 plus a small project as a
graphics-focused unit. Longer offerings can add deeper lectures on modern MMIO,
USB HID, graphics APIs, audio scheduling, or embedded Linux.

## Topic Coverage

| Topic | Where it appears |
| --- | --- |
| I/O peripherals and operation modes | Labs 1-7 |
| Polling and interrupt-style handling | Labs 2-4, project loop |
| IA-32/PC historical device interfaces | Labs 1-5, UART |
| Direct memory mapping ideas | Lab 5 framebuffer, Lab 6 audio buffer |
| C structure and parameter passing | every lab API |
| Event-driven programming and state machines | Labs 2-5, project |
| Static libraries and reusable helpers | Labs 2 onward |
| Systematic debugging | tests, traces, dumps, WAVs, replays |
| Tooling | `cc`, `make`, Git, generated tests, docs |

## Recommended Bibliography

Elecia White's *Making Embedded Systems* is a strong primary companion because
it treats embedded software as design work rather than only register trivia.
Derek Molloy's *Exploring Raspberry Pi* is useful when instructors want a bridge
to Linux-based hardware interfacing. Selected chapters from *Operating Systems:
Three Easy Pieces* can support the process, virtualization, and I/O context.

The Machine Lab guides also link to focused external references for CMOS/RTC,
PIT timers, PS/2 input, VBE graphics, PCM audio, and UART programming.

## Assessment Shape

For an LCOM-like semester, start from 40% theory / 60% project. The theory
component should check conceptual understanding of device access, C behavior,
event loops, and debugging methodology. The project component should reward
working integration, clear architecture, reproducible artifacts, and meaningful
use of multiple device areas.

For the project studio, tell students early that the expected final delivery is
application source, build/run instructions, project documentation, a short
architecture report, and a short demo video. Teams can share the artifact, but
each student should be assessed on their own progress and technical
understanding.

See the [project rubric](/instructors/rubric) for a more concrete breakdown.
