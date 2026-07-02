# Lab 2 Check

Run the focused timer tests before trying to use the timer from a larger
program. They verify the controller-facing pieces directly: divisor arithmetic,
status reads, interrupt subscription, and tick accounting. Then run a small
headless example to confirm that the timer behaves correctly in an application
loop.

```sh
machinelab test timer
machinelab run --headless -- build/examples/timer_int 3
```

## Discussion Prompts

When discussing the result, focus on policy. Why does the timer API expose
`timer_ticks()` instead of sleeping directly? What changes if your game updates
at 60 Hz but the timer fires faster? Which timing values should be constants,
and which ones should be configurable by a final project?

## External Reading

The LCOM chapters on the
[timer controller](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab2/timer-controller.html),
[interrupts](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab2/interrupts.html),
and [timer libraries](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab2/library.html)
are good companion readings. For broader background, review the OSDev pages on
the [Programmable Interval Timer](https://wiki.osdev.org/Programmable_Interval_Timer)
and [interrupts](https://wiki.osdev.org/Interrupts). The GNU C notes on
[integer types](https://www.gnu.org/software/libc/manual/html_node/Integers.html)
are useful when divisor arithmetic behaves unexpectedly.
