# How It Works Today

Modern machines still need timer interrupts, but they rarely rely on the PIT as
the main timing source.

## Modern Timer Sources

Common timer facilities include APIC timers, HPET, invariant TSC and TSC
deadline timers, paravirtualized clocks in VMs, scheduler timers, and
high-resolution timers. The operating system chooses the best source for the
platform and hides most of that choice from ordinary applications.

## What Changed

Modern timers are more precise, more scalable across CPUs, and better integrated
with power management. Interrupt routing also changed: APIC and MSI/MSI-X
replaced much of the older PIC-style model.

Another major change is that modern systems care deeply about power. Waking a
CPU too often wastes energy, so kernels coalesce timers, choose tickless idle
strategies, and avoid unnecessary periodic interrupts when possible. This is far
beyond Lab 2, but it explains why real timing APIs sometimes behave differently
from a simple "interrupt every N milliseconds" mental model.

## What Stayed The Same

The event-loop lesson remains. Time arrives as events, handlers should be
small, application state should advance predictably, and tests need deterministic
time. Machine Lab keeps the older PIT-shaped interface because it is small
enough to program directly and clear enough to teach.

Once you understand this older shape, modern APIs such as `timerfd`, browser
animation callbacks, SDL timers, or game-engine fixed timestep loops become
easier to reason about. They are higher-level interfaces over the same basic
problem: something has to decide when the program should advance.
