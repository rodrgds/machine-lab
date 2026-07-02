# From Minix And LCF To Machine Lab

This comparison is based on the previous group implementation under
`~/uni/lcom/MINIX/shared`: seven device labs and the Ninjix final project.

## The Core Change

The old project mixed two kinds of work:

1. transferable systems concepts: registers, bit fields, IRQ-driven state,
   device protocols, memory layout, graphics, and application architecture;
2. Minix/LCF ceremony: service startup, IPC notification decoding, VM paths,
   wrapper logging, and restoring guest-driver state.

`Machine Lab` retains the first category and replaces the second with a small
portable runtime API.

## Direct Mapping

| Previous Minix/LCF pattern | Machine Lab pattern | Learning status |
| --- | --- | --- |
| `lcf_start` / lab callback dispatch | normal `main` | removed ceremony |
| `sef_startup` and service lifecycle | `lcom_init` / `lcom_exit` | simplified |
| `driver_receive` plus IPC source checks | `lcom_event_wait` | same event-loop reasoning |
| `sys_irqsetpolicy` / hook IDs | `lcom_irq_subscribe` / IRQ masks | preserved and safer |
| `sys_inb` / `sys_outb` wrappers | typed `lcom_port_read*` / `write*` | preserved |
| `vm_map_phys` and Minix privileges | `lcom_phys_map` | mapping/pointer work preserved |
| `vg_init` / LCF graphics helpers | VBE info, set mode, map, present | framebuffer work preserved |
| `/home/lcom/labs/...` output paths | normal host paths/options | host-native |
| manual VM input | live SDL or replay scripts | expanded |
| two VMs and a serial/null-modem setup | `machinelab run-pair` | protocol retained, setup removed |

## What Students Still Learn

- advanced C: fixed-width integers, promotion, pointers, output parameters,
  varargs, memory layout, and modular interfaces;
- bitwise register programming and read-modify-write operations;
- interrupt subscription, masks, event loops, and small handlers;
- i8254 divisors and status read-back;
- i8042 status flow, keyboard scancodes, PS/2 commands, and mouse packets;
- RTC register selection, UIP checks, and BCD conversion;
- VBE modes, pitch, framebuffer offsets, color formats, XPMs, and sprites;
- PCM layout, sample synthesis, and audio control registers;
- UART configuration, FIFOs, loopback, interrupts, framing, and protocols;
- application state machines, deterministic simulation, rendering separation,
  Makefiles, and reusable C libraries.

The ported Ninjix example is useful evidence: its gameplay architecture,
device-facing libraries, IRQ dispatch, framebuffer renderer, and multiplayer
protocol remain. The outer process/event plumbing changed.

## What Students No Longer Learn Here

- Minix service framework and SEF lifecycle APIs;
- Minix IPC message structures, endpoint filtering, and `driver_receive`;
- Minix-specific IRQ policy calls and privilege setup;
- LCF command dispatch, print helpers, trace wrappers, and compatibility rules;
- VM image management, VirtualBox networking, shared folders, Guest Additions,
  display resizing, mouse capture, and fixed guest filesystem paths;
- cleanup whose purpose is restoring the VM's own keyboard/mouse driver.

These are valid Minix administration topics, but they are not necessary to
teach the device and C concepts above. If Minix service development is itself a
course outcome, it should remain a separate explicit exercise rather than an
incidental prerequisite for every hardware lab.

## What Students Learn Instead

- runtime/backend separation and a narrow process protocol;
- deterministic tests for interactive programs;
- authored and recorded replays for reproduction, assessment, and demos;
- trace, frame, WAV, and video artifacts as observable evidence;
- two-endpoint serial testing on one host;
- normal sanitizers, debuggers, editors, Git hooks, and CI;
- packaging a final project as a host application that remains runnable after
  the course.

## Trade-offs

`Machine Lab` is an educational device model, not a general operating system. It
does not teach process servers, kernel/user privilege boundaries as implemented
by Minix, real hardware timing, DMA, or physical bus discovery. The current
devices intentionally model the parts exercised by the labs.

That limitation should be stated directly: this approach is stronger for the
course's C/device/project loop, but it is not a substitute for a dedicated OS
internals course or real-driver laboratory.
