# Architecture

## Boundary

```text
student C process
  libmachinelab client
        | local protocol
        v
  machinelab runtime server
        |
        +-- bus and IRQ controller
        +-- virtual devices
        +-- headless or SDL/audio backend
```

The student binary is a normal process linked with `libmachinelab`. `machinelab run`
starts it with a local runtime channel and owns the virtual machine. Port reads,
port writes, IRQ subscriptions, event waits, physical mappings, and VBE calls
cross that boundary through a small protocol.

This separation is intentional:

- student code cannot accidentally depend on SDL;
- the same binary runs interactively and deterministically;
- crashes and output remain isolated at the process boundary;
- device behavior can be traced and tested independently;
- host backends can evolve without changing lab code.

## Public Student Surface

`sdk/include/lcom/lcom.h` is copied into student workspaces as
`include/lcom/lcom.h` and exposes:

- initialization and console output;
- 8/16/32-bit port I/O;
- IRQ subscribe/unsubscribe and event waits;
- virtual physical memory map/unmap;
- VBE mode info, mode changes, and frame presentation.

Device headers add register addresses and constants. AC97 also exposes PCM
buffer metadata. There are no Minix IPC structures, LCF callbacks, SDL objects,
or host pointers in the public contract.

## Virtual Devices

| Device | Student-visible concepts | Host outputs |
| --- | --- | --- |
| i8254 PIT | control words, divisors, read-back, IRQ0 | virtual ticks |
| i8042 | status/data ports, keyboard IRQ1, mouse IRQ12 | live/scripted input |
| RTC/CMOS | register selection, UIP, BCD/binary fields | live or fixed time |
| VBE | mode info, framebuffer mapping, pitch, present | SDL window or PPM |
| AC97-lite | PCM mapping, rate, byte count, controls | SDL, null, or WAV |
| UART16550 | DLAB, LCR/FCR/IER/LSR/MCR, IRQs | loopback/local/paired cable |

The models focus on the course-visible behavior. They do not claim cycle-level
hardware emulation.

## Time And Determinism

Headless execution uses a 60 Hz virtual clock advanced by runtime/guest events.
Scripts inject input at virtual timestamps. A fixed RTC, traces, screenshots,
frame sequences, WAV files, and replay video all derive from the same run.

SDL execution normally follows host time and maps live keyboard/mouse input to
virtual i8042 data. The guest program and virtual device interfaces are
otherwise unchanged.

## Testing Layers

1. `unit_tests` checks buses, controllers, devices, scripts, and core behavior.
2. `labN_solution` executables run each reference solution through the runtime.
3. `tests/integration.sh` exercises CLI errors/help, examples, artifacts,
   pairing, generated workspaces, replay encoding, and bundles.

The complete gate is:

```sh
machinelab-test
```
