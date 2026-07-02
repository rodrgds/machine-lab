# Adopt Machine Lab

This guide is for instructors, teaching assistants, club organizers, and
curriculum leads who want to use Machine Lab quickly without turning the first
week into infrastructure work.

Machine Lab works best when it is presented as a practical course kit: students
learn C by interacting with device-shaped interfaces, then combine those
interfaces into a final reactive project. The model is inspired by
computer-laboratory courses such as LCOM, but it removes the VirtualBox/Minix
setup from the critical path and makes the material portable enough for
independent study, workshops, and mixed-platform classrooms.

## Course Profile

A full adoption can fit a 6 ECTS course with about 52 contact hours and 162
total hours of student work.

| Course measure | Suggested value |
| --- | --- |
| Credits | 6 ECTS |
| Contact time | 52 contact hours |
| Total work | 162 total hours |

A typical shape is two hours of lecture or concept discussion plus two hours of
laboratory work per teaching week, followed by a project studio period.

The course is strongest for students who have already seen introductory
programming, Computer Architecture, and Operating Systems. They do not need to
have written a kernel or a real driver before. The labs introduce the relevant
device-facing habits gradually: bit fields, status registers, polling,
interrupt-style events, memory mapping, buffers, byte streams, and state
machines.

The teaching method should be learning by doing. Short theory sessions explain
the device model and the C technique needed for the next lab. Lab sessions then
leave implementation gaps that students must fill, test, debug, and eventually
reuse in the project.

## Adoption Modes

| Mode | Duration | Recommended scope |
| --- | --- | --- |
| Workshop | 2 to 6 hours | Lab 1 plus one visual example |
| Course module | 2 to 4 weeks | Labs 1, 2, 3, 5 and a small project |
| Semester course | 10 to 14 weeks | Labs 1 to 7 and final project studio |
| Replacement lab track | existing systems course | Select labs that match your lecture order |

For a semester course, use the labs as the first half of the term and the
project as the second half. That mirrors a proven laboratory-course structure:
students first build reusable device helpers, then use those helpers in a
larger program of their own.

For a short module, resist assigning every lab. A tight four-week version can
cover RTC/bitwise work, timers, keyboard input, framebuffer graphics, and a
small scripted demo. That still teaches the event-loop shape without forcing the
staff to grade a full project.

## LCOM-Style Project Model

If you are adopting Machine Lab for a full semester, the final project should
feel like the reason the labs existed. Students should not be asked to bolt a
random interface onto isolated exercises. They should build a visual,
interactive C application that uses the device areas studied during the term:
timer, keyboard, mouse, graphics, RTC, audio, and optionally a serial link.

The project topic can stay open. Games work well because they force timing,
input, rendering, and state management to meet in one loop, but they are not
the only useful outcome. Text editors, drawing tools, calendars, music
sequencers, simulations, protocol demos, and small collaborative applications
can all be good projects when they use devices for a real purpose.

The useful constraint is not "make it large." It is "make it explainable."
Students should be able to describe what each device does in the application,
how events enter the system, how state changes, how rendering happens, and how
the final program can be rebuilt and demonstrated by staff.

### Teams And Individual Assessment

Teams of four are a practical default for a project studio. They are large
enough for device ownership, application state, rendering, and documentation to
be split, but still small enough that every member should understand the whole
program.

Make it explicit that the artifact is shared but the grade is individual. Each
student should be able to run the project, explain a meaningful part of the
architecture, justify device choices, and discuss the code they contributed.
This keeps group work from turning into a single-implementer submission.

### Proposal

Ask each team to submit a short project proposal before the project studio
starts, typically around the transition from labs to project work. The proposal
does not need to be long. It should answer:

What are we building? Which devices will the project use? What is the smallest
finished loop that would count as a working project? What parts are risky? What
will we cut first if the scope becomes too large?

This is the right moment for staff to challenge projects that are too vague,
too UI-heavy without device integration, too dependent on assets, or too
ambitious for the time available.

### Final Delivery

For the final deadline, require every artifact to be committed or uploaded
before the cutoff. A simple public rule is that the assessed version is the last
commit before the deadline. That avoids ambiguity and encourages students to
treat packaging as part of the engineering work.

Require at least these artifacts:

| Artifact | Purpose |
| --- | --- |
| Application source | the complete buildable project |
| Build/run instructions | the commands staff should execute |
| Project documentation | controls, dependencies, known limitations, repository structure |
| Short report | goal, architecture, devices, differentiating features |
| Demo video | quick proof that the submitted version runs |

For the report, five pages is a useful ceiling. It forces teams to explain the
architecture instead of writing a diary. Ask for an architecture image, a device
usage table, and a short discussion of what makes the project different from a
minimal lab exercise.

For the video, five minutes is enough. The demo should show the normal user
flow, the device interactions that matter, and one short explanation of the
technical structure. It should not replace the source submission.

### Project Entry Point

If your course still uses an LCF-compatible shape, students can keep the same
mental model: one project entry point, a configured build file, and a command
that launches the application. In Machine Lab, the names can be simpler, but
the expectation is the same: staff should not need to reverse-engineer how to
start the program.

For example, the project contract can be written as:

```c
int proj_main_loop(int argc, char *argv[]) {
  /* application setup, event loop, cleanup */
  return 0;
}
```

In a Machine Lab workspace, the exact wrapper may differ from an LCOM/Minix
service, but the project should still have a single obvious run path, a
Makefile target, and a short smoke script when possible.

### AI Use Policy

Machine Lab is a learning exercise, so AI policy should focus on
responsibility rather than pretending the tools do not exist. A workable policy
is:

Students may use AI tools to support learning, debugging, planning, and code
review, but they remain responsible for every submitted file. They must be able
to explain and justify the code. They must review, test, and adapt generated
output. They must not share confidential or personal data with AI tools. They
must not submit complete generated solutions as unaided work. If asked, they
must identify which tools contributed to which parts of the project and how
those outputs were verified.

That language gives staff room to ask good oral questions: "Why is this buffer
double-buffered?", "What happens if the mouse packet loses synchronization?",
"Which part came from a tool and how did you test it?" The student is assessed,
not the tool.

### Device-Weighted Rubric

For an LCOM-like project component, a concrete rubric can make expectations
less mysterious:

| Area | Weight |
| --- | ---: |
| Practical-class individual progress | 10% |
| RTC usage | 5% |
| Timer usage | 5% |
| Keyboard usage | 10% |
| Mouse usage | 10% |
| Graphics card / framebuffer usage | 20% |
| Code structure and maintainability | 10% |
| Demonstration and discussion | 20% |
| Report and video | 10% |
| Optional serial-port extension | up to 10% extra |

The weights are less important than the message they send. Graphics usually
deserves the largest device weight because it dominates the user-visible
program and forces students to think about framebuffers, drawing policy, and
performance. Keyboard and mouse deserve separate credit because they test
different input models. Timer credit should reward real timing policy, not just
sleeping. RTC credit should reward a purposeful use, such as timestamps,
calendar logic, saves, logs, or time-based behavior.

When adapting this rubric, keep "sound usage" as the standard. A project that
uses fewer devices well is usually stronger than a project that touches every
API superficially. Extra credit for serial communication works best when it
enables a real feature: two-player communication, a chat/debug console, a
protocol visualizer, or a paired controller.

## One-Week Adoption Plan

### Day 1: Pick The Course Shape

Decide whether Machine Lab is a workshop, a module, a semester lab track, or a
replacement for an existing practical sequence. Publish that scope early so
students understand whether the labs are practice, graded assignments, or
building blocks for a final project.

Also decide what students must submit. Common evidence includes lab source,
test output, traces, screenshots, WAV files, replay scripts, and final project
bundles. The best artifacts are reproducible; a staff member should be able to
run the same command and observe the same result.

### Day 2: Verify The Toolchain

On an instructor machine, run:

```sh
machinelab --help
machinelab setup student-smoke --force
make -C student-smoke
machinelab test rtc --project student-smoke
```

The last command should fail for a fresh workspace because the TODO stubs are
incomplete. That is a good sign: students will see the same initial state.

> [!IMPORTANT]
> Do not distribute the upstream repository as the normal student workspace.
> Generate workspaces with `machinelab setup` so students see the course surface
> rather than runtime internals.

### Day 3: Pilot One Lab

Have one staff member follow Lab 1 as a student, without reference solutions.
They should read the guide, run the failing test, implement the bit helpers,
re-run the test, and write down every confusing step. Patch the handout, LMS
note, or local setup instructions before students see it.

This pilot is worth the time. Most adoption failures come from small mismatches:
a stale command, an unclear generated folder, a missing compiler, or a handout
that assumes too much about C pointers.

### Day 4: Publish Assessment And Policy

For an LCOM-like course, a reasonable assessment split is 40% theory / 60%
project. Use the phrase 40% theory / 60% project in your public course plan if
you want students to understand the emphasis immediately. The theory component
can check device concepts, C reasoning, event loops, memory layout, and
debugging methodology. The project component should reward working integration,
not only feature count.

Publish the lab schedule, late policy, collaboration policy, AI policy, project
milestone dates, artifact expectations, and grading rubric before project work
begins. In a full semester, students should know the proposal deadline, the hard
final delivery deadline, what counts as the assessed commit, and whether the
video/report are submitted through an LMS, repository, or both.

See the [syllabus map](/instructors/syllabus) and
[project rubric](/instructors/rubric) for a starting point. If you are adapting
from an LCOM-style course, copy the structure of the requirements but rewrite
the dates, submission channels, and device list for your local setup.

### Day 5: Run The First Class

Show a short demo, generate a workspace live, run `make`, run a failing lab
test, and explain that the failure points to missing student work rather than a
broken setup. Then implement one tiny bit helper together and stop before
solving the whole lab.

The strongest first impression is not a perfect lecture. It is students seeing
a failing test, changing C code, and producing a different machine-facing result
within the first session.

## Mapping To A Traditional Computer Laboratory Course

Machine Lab can cover the same teaching territory as a peripheral-programming
laboratory course without requiring a microkernel VM. The old setup is still
valuable context: Minix, privileged services, and direct I/O permissions show
how operating systems mediate hardware access. Machine Lab keeps the device
contracts and makes them easier to run on modern student machines.

| Traditional objective | Machine Lab equivalent |
| --- | --- |
| Use hardware interfaces of common peripherals | Use RTC, PIT, i8042, VBE, audio, and UART-shaped APIs |
| Develop low-level C software | Implement small libraries with explicit error handling |
| Practice polling and interrupts | Use deterministic event loops and runtime IRQ-style delivery |
| Build static libraries for later reuse | Move lab helpers into reusable modules for the project |
| Debug systematically | Use tests, traces, frame dumps, WAVs, and replay scripts |
| Build a final interactive project | Combine timers, input, graphics, audio, and serial protocols |

## Suggested Prerequisites

Students should be comfortable compiling C, reading function prototypes,
debugging small programs, and reasoning about memory. They should have seen the
basic ideas from Computer Architecture and Operating Systems: registers, memory
layout, interrupts, processes, and system calls.

If students are weaker in C, spend the first session on bit operations,
pointers, structs, `make`, and reading compiler errors. If students are stronger
in systems work, move faster through Lab 1 and spend more time on project
architecture and deterministic replay.

## Public Marketing

Machine Lab should be marketed as reusable courseware, not as a flashy
emulator. Lead with the adoption value: free course kit, portable setup, C labs
with deterministic tests, and final projects that produce visible artifacts.

Good first channels are Hacker News, `r/C_Programming`, `r/learnprogramming`,
`r/osdev`, `r/compsci`, SIGCSE-adjacent teaching groups, and department mailing
lists for computer architecture or operating systems instructors.

Do not lead with compatibility history. Mention it only after the value is
clear: Machine Lab keeps the useful device-facing ideas from older PC lab work
and makes them portable.

## One-Week Checklist

- `machinelab` runs on every supported student platform.
- A generated workspace compiles with `make`.
- Staff can explain generated workspace versus upstream repository.
- First lab handout is published.
- Staff know which files are reference solutions.
- Help channel and issue-report format are published.
- A small demo is ready: word processor, music maker, or game replay.
- Project rubric is visible before students start combining devices.

## Reuse And Attribution

Code is MIT licensed. Course docs, lab handouts, screenshots, and demo media are
CC BY 4.0 licensed. Universities may copy and adapt the material as long as they
credit Machine Lab and contributors.
