# Lab 2 Context: Why PC Timers Exist

The CPU is much faster than many I/O devices and much faster than human-visible
time. A program needs a way to ask "has enough time passed?" without blocking
the whole machine in a busy loop.

Older PCs used dedicated timer hardware such as the i8254 PIT. The timer was
simple: program a divisor, receive periodic events, and count ticks.

## What You Learn

The most important idea is that hardware time is usually derived from a base
clock. The program does not ask the PIT for "three seconds" directly. It
programs a divisor so that a counter produces events at a requested frequency,
then it counts those events. This makes time measurable in a way that is
independent from how fast the CPU happens to execute a loop.

You will also see another recurring device pattern: programming a controller
often requires writing several bytes in a required order. A timer divisor is a
16-bit value, but the interface writes it as a low byte and a high byte. If the
order is wrong, the device can be configured to a completely different value
while every individual port write still "succeeds."

## Why This Comes Before Graphics

Animation is not only drawing. A program needs a timing policy:

when to update state, when to draw, how long to wait for input, and when to
stop an idle loop. Without that policy, a graphical program either runs as fast
as the CPU allows or sleeps in a way that is difficult to test.

Lab 2 supplies the first version of that policy layer. The implementation is
small, but the design habit is important: the handler records ticks, and the
main program decides what those ticks mean.
