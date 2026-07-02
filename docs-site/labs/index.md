# Lab Track

The lab track teaches machine-facing C through small device-shaped exercises.
Each lab is split into:

- overview: what the device model is;
- tasks: what you implement and what gaps are intentionally left;
- check: commands, prompts, and external reading.

## Sequence

| Lab | Device idea | Check |
| --- | --- | --- |
| [Lab 1](/labs/lab1/) | bit masks, CMOS/RTC, BCD | `machinelab test rtc` |
| [Lab 2](/labs/lab2/) | PIT divisors and timer IRQs | `machinelab test timer` |
| [Lab 3](/labs/lab3/) | i8042 keyboard and scancodes | `machinelab test kbd` |
| [Lab 4](/labs/lab4/) | PS/2 mouse packets | `machinelab test mouse` |
| [Lab 5](/labs/lab5/) | VBE framebuffer and sprites | `machinelab test graphics` |
| [Lab 6](/labs/lab6/) | AC97-lite PCM audio | `machinelab test audio` |
| [Lab 7](/labs/lab7/) | 16550 UART and serial protocols | `machinelab test uart` |

## Guide Style

Each lab now has more than a task list:

- history and motivation;
- controller model;
- protocol steps;
- implementation tasks;
- checks and discussion prompts;
- external reading;
- "how it works today" notes for modern systems.

The goal is to teach why the old PC-shaped interface works, why it looked that
way, and which ideas still transfer to modern MMIO, APIC/MSI, HID, GPU, audio,
and serial stacks.

## Method

1. Read the overview.
2. Run the failing starter test.
3. Implement the requested functions.
4. Keep reusable helpers small.
5. Produce one artifact: test output, screenshot, WAV, trace, or replay.

> [!TIP]
> Fresh stubs are expected to fail. The first failure is the starting point, not
> evidence that setup is broken.
