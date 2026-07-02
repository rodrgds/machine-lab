# Creating A Timer Library

Lab 2 is the first lab where it makes sense to create reusable helpers. Later
labs and final projects need timing for:

- animation;
- idle timeouts;
- keyboard/mouse loops;
- audio scheduling;
- replay-friendly demos.

## What Belongs In The Library

Good library candidates are subscribe/unsubscribe wrappers, tick counters,
elapsed-second helpers, frequency setup helpers, and small state reset functions
for tests. These helpers describe the timer itself and make later code easier
to read.

Poor candidates are project-specific game state, drawing code, keyboard or
mouse parsing, and hardcoded lab test output. If a function only makes sense for
one exercise transcript, it probably does not belong in the reusable library.

The library should make future code clearer without hiding the device model you
are supposed to understand.

This is a useful design tension. A library that exposes every register access is
not much of a library, but a library that hides all timing decisions prevents
students from learning how timer-driven programs work. Aim for the middle:
small functions with names that explain the policy they implement.
