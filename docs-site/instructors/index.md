# Instructor Guide

Machine Lab is designed to be adopted as a course module, workshop, independent
study track, or semester lab section. Students get a generated workspace;
instructors get lab guides, reference tests, examples, project rubrics, and a
portable runtime that avoids a VM setup bottleneck.

The full course shape is close to a traditional computer laboratory unit:
students first implement small C libraries for device-shaped interfaces, then
reuse those libraries in a final interactive project. A 6 ECTS version can use
about 52 contact hours and 162 total hours, with concept sessions supporting
the lab work rather than replacing it.

## Course Promise

Machine Lab gives students direct practice with bit fields, status registers,
polling, interrupt-style events, mapped framebuffers, audio buffers, byte
streams, static libraries, and state-machine design. It gives instructors a
course surface that can be used in a classroom, in a short module, or as
self-study material without asking students to maintain the upstream runtime.

The intended teaching method is learning by doing. The guides explain enough of
the device model for students to proceed, but they leave implementation gaps
that require careful C code, tests, debugging, and discussion.

## Course Shape

| Phase | Material | Outcome |
| --- | --- | --- |
| Week 1 | Lab 1 | bit reasoning and RTC reads |
| Week 2 | Lab 2 | PIT divisors and timer event loops |
| Weeks 3-4 | Labs 3-4 | keyboard and mouse packet handling |
| Weeks 5-6 | Lab 5 | framebuffer graphics and sprites |
| Weeks 7-8 | Labs 6-7 | audio buffers and UART protocols |
| Weeks 9-13 | Project studio | interactive C application |

Use the [syllabus map](/instructors/syllabus) for a detailed week-by-week
version, the [adoption guide](/instructors/adoption) for the first week of
setup, and the [project rubric](/instructors/rubric) for assessment.

## What To Publish

Publish the release binary or install instructions, generated workspace
instructions, lab schedule, collaboration policy, grading rubric, final project
milestones, and expected artifacts. Artifacts can include test output, traces,
screenshots, WAV captures, replay scripts, videos, or bundles.

## What To Keep Private

| Path | Reason |
| --- | --- |
| `dev/lab-solutions/` | reference implementations |
| `dev/lab-tests/` | can reveal expected behavior |
| upstream CMake/runtime internals | not part of normal student work |

Students should normally start from `machinelab setup`, not from the upstream
repository. The upstream repository is for maintainers and contributors.

## First Class Flow

Show the short demo, generate a workspace live, run `make`, run a failing lab
test, and implement one tiny bit helper together. Stop before solving the whole
lab. The first class should prove that the environment works and that tests are
feedback for missing student code, not mysterious infrastructure failures.

Next: [adopt Machine Lab in one week](/instructors/adoption).
