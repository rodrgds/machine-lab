# How It Works Today

Modern machines still have compatibility paths for old PC interfaces, but most
high-performance hardware is not controlled through isolated I/O ports.

## MMIO

Modern devices commonly expose registers through memory-mapped I/O. Instead of
special port instructions, software reads and writes addresses in a physical
memory range reserved for the device. The syntax looks more like pointer access
than port access, but the programmer still has to respect the device contract.
Status bits report readiness and errors. Control bits configure behavior.
Read-modify-write operations still matter because neighboring fields often share
the same word. Ordering rules still matter because devices can observe writes
in a different way than normal memory.

The access mechanism changed; the protocol discipline did not.

## Firmware And OS Mediation

Normal user programs usually call operating-system APIs such as
`clock_gettime`. The OS, firmware, and hardware abstraction layers decide how to
read time on that machine. It may involve RTC state, HPET, APIC timers, TSC
deadline timers, or paravirtualized clocks.

That stack is good engineering. Most applications should not know which clock
source won a platform-specific selection process. A browser, game, database, or
editor wants a stable time API. The device-specific work belongs in the kernel,
firmware, hypervisor, or runtime layer.

## Why Learn The Old Shape?

The RTC is small enough to understand completely. It teaches register selection,
status-before-data, binary encoding choices, and output-parameter error
handling without making you learn a modern kernel driver framework first. Those
lessons transfer to real drivers even when the bus changes from port I/O to
MMIO, and they also transfer to ordinary software that has to parse small binary
protocols correctly.
