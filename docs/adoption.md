# Adoption Guide

This guide is for instructors, teaching assistants, club organizers, and
curriculum leads who want to use Machine Lab quickly without turning the first
week into infrastructure work.

The public website is the canonical version of this guide:
<https://machine-lab-docs.pages.dev/instructors/adoption>

## Course Profile

Machine Lab works best as practical courseware. Students learn C by interacting
with device-shaped interfaces, then combine those interfaces into a final
reactive project. The model is inspired by computer-laboratory courses such as
LCOM, but it removes the VirtualBox/Minix setup from the critical path.

A full adoption can fit a 6 ECTS course with about 52 contact hours and 162
total hours of student work. A typical shape is two hours of lecture or concept
discussion plus two hours of laboratory work per teaching week, followed by a
project studio period.

Students should have already seen introductory Programming, Computer
Architecture, and Operating Systems. The teaching method should be learning by
doing: short theory sessions explain the device model, then lab sessions leave
implementation gaps for students to fill, test, debug, and reuse.

## Use Machine Lab In One Week

Minimum viable adoption means students install or receive `machinelab`, generate
a workspace with `machinelab setup`, compile one starter lab, run one starter
test that fails in the expected way, and receive the first handout plus grading
expectations.

### Day 1: Pick Scope

| Mode | Best for | Use |
| --- | --- | --- |
| Workshop | 2 to 6 hours | Lab 1 plus one visual example |
| Course module | 2 to 4 weeks | Labs 1, 2, 3, 5 and a small project |
| Semester course | 10 to 14 weeks | Labs 1 to 7 and final project studio |
| Replacement lab track | existing systems course | Select labs that match your lecture order |

### Day 2: Verify The Toolchain

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
re-run the test, and write down every confusing step.

### Day 4: Publish Assessment And Policy

For an LCOM-like course, a reasonable assessment split is 40% theory / 60%
project. Publish the lab schedule, late policy, collaboration policy, AI policy,
project milestone dates, artifact expectations, and grading rubric before
project work begins. Students should know the proposal deadline, the hard final
delivery deadline, what counts as the assessed commit, and whether the
video/report are submitted through an LMS, repository, or both.

### Day 5: Run The First Class

Show a short demo, generate a workspace live, run `make`, run a failing lab
test, and explain that the failure points to missing student work rather than a
broken setup. Then implement one tiny bit helper together and stop before
solving the whole lab.

## Marketing Position

Machine Lab should be marketed as reusable courseware, not as a flashy emulator.
Lead with the adoption value: free course kit, portable setup, C labs with
deterministic tests, and final projects that produce visible artifacts.

Good first channels are Hacker News, `r/C_Programming`, `r/learnprogramming`,
`r/osdev`, `r/compsci`, SIGCSE-adjacent teaching groups, and department mailing
lists for computer architecture or operating systems instructors.

## LCOM-Style Final Project Template

For a full semester, the project should ask teams to build a visual,
interactive C application that combines the device areas studied in the labs:
timer, keyboard, mouse, graphics, RTC, audio, and optionally a serial link.
Games are natural, but editors, drawing tools, calendars, music sequencers,
simulations, and protocol demos are also good fits.

Teams of four are a practical default, but the shared artifact should not imply
identical grades. Each student should be able to run the project, explain the
architecture, justify device choices, and discuss their own contribution.

Require a proposal before project studio starts. It should describe the idea,
the devices used, the smallest finished loop, and the main risks. For final
delivery, require source, build/run instructions, project documentation, a
short report, and a short demo video. The assessed version should be the last
commit before the hard deadline.

A compact report prompt works well:

- What was the goal and what does the application do?
- How is the project structured? Include an architecture image.
- Which devices are used, and for what purpose?
- What differentiates this project from a minimal lab exercise?

AI policy should be explicit. Students may use tools to support learning,
debugging, planning, and review, but they remain responsible for every
submitted file. They must understand and be able to explain their code, review
and test generated output, avoid sharing confidential data, avoid presenting
complete generated solutions as unaided work, and disclose tool usage if asked.

An LCOM-style project rubric can use this shape:

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

The standard should be sound usage, not shallow API coverage. A project that
uses fewer devices coherently is stronger than one that touches every device
without a clear reason.

## Reuse And Attribution

Code is MIT licensed. Course docs, lab handouts, screenshots, and demo media are
CC BY 4.0 licensed. Universities may copy and adapt the material as long as they
credit Machine Lab and contributors. See [course/LICENSE.md](../course/LICENSE.md).
