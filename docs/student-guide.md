# Student Guide

This guide is for people using a generated Machine Lab workspace. You do not
need to work inside the upstream Machine Lab repository.

Machine Lab asks you to write C code that talks to machine-like devices:
registers, ports, interrupts, framebuffers, audio buffers, and serial links. The
runtime is portable, but the habits are the same ones used when reading real
hardware manuals: inspect bits, respect status flags, handle partial data, and
keep event loops predictable.

## Start Here

```sh
machinelab setup student
cd student
make
machinelab test rtc
```

Fresh lab stubs compile but fail checks until you implement the requested
functions. A failing first test is expected.

> [!NOTE]
> Your workspace is intentionally smaller than the upstream repository. It
> contains the public headers, starter lab files, tests, and a small project
> area. Runtime internals and release tooling are not part of the normal student
> workflow.

## Edit These

| Path | Use |
| --- | --- |
| `labs/<device>/*_lab.c` | required lab implementations |
| `labs/<device>/include/` | function contracts for that lab |
| `lib/<device>/` | optional reusable helpers you build over time |
| `proj/` | final-project or experiment code |
| `Makefile` | build wiring, only when your instructor asks for it |

## Usually Ignore These

| Path | Why |
| --- | --- |
| `include/lcom/` | copied SDK headers; treat as read-only |
| `.mlab/` | generated tests and build artifacts |
| `build/` | generated object files and binaries |
| upstream `runtime/`, `common/`, `.github/` | Machine Lab developer internals |
| `dev/lab-solutions/` in the upstream repo | instructor/developer reference code, not starter work |

## Lab Loop

Use the same loop for every lab:

```sh
make
machinelab test rtc
```

1. Read the handout.
2. Run the failing test once.
3. Find the requested function signatures in the lab header.
4. Implement the smallest correct version.
5. Re-run the test.
6. Move repeated helper logic into `lib/<device>/` only after the tested entry
   point works.

> [!TIP]
> Keep a small note for each register you touch: address, readable bits, writable
> bits, and the meaning of each status flag. Later labs reuse the same reasoning
> pattern even when the device changes.

## How To Read A Lab

Every lab handout has:

- a device model: what the controller exposes;
- requested functions: the exact API the tests call;
- guided gaps: the parts you must design and implement;
- common mistakes: bugs that look plausible but fail under tests;
- discussion prompts: questions that prepare you for the final project.

The handout should teach enough to get started. It should not give you a full
solution.

## Debugging

Useful commands:

```sh
make clean
make
machinelab test graphics
machinelab run --headless --dump-frame build/frame.ppm -- build/student_project
machinelab run --headless --audio-wav build/out.wav -- build/student_project
```

When a test fails:

- read the first failing check, not only the last line;
- print intermediate register values in hexadecimal;
- check whether the function should return an error instead of writing partial
  output;
- confirm you are not reading or writing outside a buffer;
- create a tiny helper only when it makes the next call clearer.

## Final Project Loop

The final project should combine several device areas into one reactive C
application:

```sh
make run
machinelab run build/student_project
```

Good projects combine at least three areas:

- timer + keyboard + framebuffer for an arcade loop;
- mouse + framebuffer for drawing, editing, or UI tools;
- audio + timer + keyboard for sequencers or sound toys;
- UART + `run-pair` for two-player or protocol work.

Your project should have:

- a minimum finished loop that can be demoed early;
- one deterministic replay script;
- one visible artifact, such as a screenshot, WAV, trace, video, or bundle;
- a short note explaining which devices you used and how the event loop works.

## Public API Boundary

Student code should include headers from `include/lcom/` and lab headers. It
should not include SDL, runtime C++ headers, Machine Lab internal protocol
files, or files from `dev/lab-solutions/`.

## Help Requests

When asking for help, include:

- the lab name;
- the command you ran;
- the first failing test output;
- the function you think is wrong;
- a short explanation of what you expected.

This makes it possible for another person to help without taking over the
implementation.
