# Student Guide

This page is for people using a generated Machine Lab workspace. You do not
need to work inside the upstream Machine Lab repository.

## Start Here

```sh
machinelab setup student
cd student
make
machinelab test rtc
```

Fresh lab stubs compile but fail checks until you implement the requested
functions. A failing first test is expected.

## What You Edit

| Path | Use |
| --- | --- |
| `labs/<device>/*_lab.c` | required lab implementations |
| `labs/<device>/include/` | function contracts for that lab |
| `lib/<device>/` | reusable helpers you build over time |
| `proj/` | final-project or experiment code |
| `Makefile` | build wiring when your instructor asks for it |

## What You Usually Ignore

| Path | Why |
| --- | --- |
| `include/lcom/` | copied SDK headers; treat as read-only |
| `.mlab/` | generated tests and build artifacts |
| `build/` | generated object files and binaries |
| upstream `runtime/`, `common/`, `.github/` | Machine Lab developer internals |

## Lab Loop

1. Read the lab overview.
2. Run the failing test once.
3. Find the requested function signatures.
4. Implement the smallest correct version.
5. Run the test again.
6. Move repeated logic into `lib/<device>/` only after the lab entry point works.

> [!TIP]
> Keep a tiny note for each register: address, readable bits, writable bits, and
> status flags. That habit carries across every lab.

## Final Project

Good projects combine at least three device areas:

- timer + keyboard + framebuffer for an arcade loop;
- mouse + framebuffer for drawing or editing;
- audio + timer + keyboard for sequencers;
- UART + `run-pair` for two-player or protocol work.

Your project should include source, a deterministic replay or scripted smoke
path, and one artifact such as a screenshot, WAV, video, trace, or bundle.
