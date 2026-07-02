# Lab 2: PIT And Timer IRQs

Generated folder: `labs/timer/`

Lab 2 introduces periodic hardware events. Instead of asking a high-level OS
clock for time, you configure a timer-like device and react to interrupts.
This is the first point where the course starts to feel reactive: the program
does not simply run a calculation and exit, it waits for a device to say that
something happened.

## Device Model

| Part | Role |
| --- | --- |
| Timer 0 | periodic interrupt source |
| Control word | selects timer, access mode, operating mode, BCD/binary |
| Divisor | converts base clock frequency into requested rate |
| IRQ line | event path from device to program |
| Interrupt handler | records that a tick happened |

The timer is deliberately simple, but it creates the foundation for almost
everything that follows. Keyboard idle timeouts, mouse polling windows, graphics
animation, audio scheduling, serial retries, and final project game loops all
need a way to measure progress through time.

## Plan

Begin with [why PC timers exist](/labs/lab2/context), then study the
[i8254 controller](/labs/lab2/timer-controller) and the arithmetic behind
[timer divisors](/labs/lab2/programming). After that, compare
[polling and interrupts](/labs/lab2/interrupts), because this choice will return
in the keyboard and mouse labs. The last part of the lab is about turning the
exercise into a [reusable timer library](/labs/lab2/library), completing the
[implementation tasks](/labs/lab2/tasks), and comparing the model with
[modern timers](/labs/lab2/modern).

> [!TIP]
> Timer code is infrastructure. Keep it small now so your final project can use
> it without copying lab-specific logic.
