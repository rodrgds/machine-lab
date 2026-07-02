# From Minix And LCF To Machine Lab

Machine Lab keeps the transferable systems ideas from older Minix/LCF lab work
and removes the setup ceremony that is not the main teaching target.

## Direct Mapping

| Previous pattern | Machine Lab pattern | Learning status |
| --- | --- | --- |
| `lcf_start` callback dispatch | normal `main` | removed ceremony |
| `sef_startup` service lifecycle | `lcom_init` / `lcom_exit` | simplified |
| `driver_receive` IPC loop | `lcom_event_wait` | event-loop reasoning preserved |
| `sys_irqsetpolicy` hook IDs | `lcom_irq_subscribe` IRQ masks | preserved and safer |
| `sys_inb` / `sys_outb` | typed `lcom_port_read*` / `write*` | preserved |
| `vm_map_phys` privileges | `lcom_phys_map` | pointer/mapping work preserved |
| `vg_init` / LCF graphics helpers | VBE info, set mode, map, present | framebuffer work preserved |
| manual VM input | SDL or replay scripts | expanded |
| two VMs and null modem | `machinelab run-pair` | protocol retained |

## What Students Still Learn

- fixed-width integers, pointers, output parameters, varargs, and modular C;
- bitwise register programming;
- IRQ subscription and event loops;
- i8254 timers;
- i8042 keyboard/mouse behavior;
- RTC register selection, UIP checks, and BCD conversion;
- VBE modes, pitch, framebuffer offsets, colors, XPMs;
- PCM layout and audio buffers;
- UART configuration, FIFOs, loopback, interrupts, framing, and protocols;
- application state machines and reusable C libraries.

## What Moves Out Of The Way

- Minix service framework and SEF lifecycle APIs;
- Minix IPC message structures;
- Minix-specific IRQ policy calls and privilege setup;
- LCF command dispatch and trace wrappers;
- VM image management, shared folders, and guest filesystem paths;
- cleanup whose purpose is restoring the VM's own driver state.

These are valid operating-systems topics. They are just not required for every
device-shaped C exercise.

## Trade-off

Machine Lab is an educational device model. It is stronger for the C/device
project loop, but it is not a substitute for a dedicated OS internals course or
real-driver laboratory.
