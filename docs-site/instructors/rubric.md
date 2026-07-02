# Project Rubric

The project should be the place where students prove that the labs became a
toolkit. A strong project is not necessarily the largest game or the prettiest
interface. It is a coherent reactive program that combines device areas, has a
clear state model, and can be demonstrated reproducibly.

## Suggested Weights

For a full semester, a useful course-level split is 40% theory / 60% project.
Inside the project component, you can either grade broad engineering outcomes
or use a device-weighted rubric. The broad version is easier to adapt:

| Area | Weight | Looks for |
| --- | --- | --- |
| Prototype milestone | 15% | minimum finished loop, realistic scope, deterministic replay |
| Final implementation | 35% | device integration, event loop, state management, polish |
| Implementation note | 10% | clear explanation of devices, timing, data flow, tradeoffs |
| Demo artifact | 5% | screenshot, WAV, video, bundle, or trace that can be reproduced |
| Individual contribution adjustment | variable | fair accounting within group work |

An LCOM-style project rubric can be more concrete about devices:

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

If you grade labs directly, reserve a separate lab component for passing checks,
helper boundaries, and correct error handling. If labs are formative, require
students to reuse their lab libraries in the project and assess the result
there.

## Strong Final Projects

| Criterion | Strong submission |
| --- | --- |
| Device use | combines at least three device areas meaningfully |
| Event loop | handles time, input, rendering, and audio without blocking surprises |
| Architecture | separates device helpers, application state, and presentation |
| Reliability | has deterministic replay or scripted smoke path |
| Finish | has a coherent start, interaction loop, and end state |
| Evidence | includes source, build instructions, and reproducible artifact |

## Suggested Milestones

The proposal should identify the project idea, the device areas it will use,
the smallest finished loop, and the main risks. The prototype should already run
and respond to input, even if the visuals or audio are temporary. The final
submission should include source, build instructions, a short implementation
note, and at least one artifact that staff can reproduce.

For a larger course, make the final delivery contract explicit. A good minimum
is application source, project documentation, build/run instructions, a report
of at most five pages, and a demo video of at most five minutes. The report
should answer what the project is, how it is structured, which devices it uses,
and what differentiates it from a minimal exercise. The video should prove that
the submitted version runs; it should not replace the source and report.

For groups, assess the shared artifact and the individual contribution. Members
of the same group can receive different marks if their contribution,
understanding, or submitted implementation notes differ significantly.

## AI Use

If AI tools are allowed, say so directly and make responsibility explicit.
Students may use tools to support learning, debugging, planning, and review, but
they remain responsible for every submitted file. They must be able to explain
the code, verify generated output, avoid confidential data disclosure, avoid
submitting complete generated solutions as unaided work, and describe tool usage
if asked.

This policy is easiest to enforce in discussion. Ask students to explain a
buffering decision, walk through a packet parser, justify a timing policy, or
show how they tested a generated section. The final grade is for the student's
understanding and artifact, not for the tool.

## What To Penalize

Penalize projects that only work through blocking sleeps, hide device code in
unreviewable blobs, cannot be rebuilt by staff, lack a reproducible demo path,
or add many features without a stable core loop. The project is a systems
exercise, so reliability and explainability matter as much as visual ambition.
