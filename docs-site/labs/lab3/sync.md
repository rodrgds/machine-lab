# Interrupts Vs Polling

Polling checks for data repeatedly. Interrupts react when a device signals that
data is available.

| Mechanism | Best use | Trade-off |
| --- | --- | --- |
| Polling | simple tests, predictable short waits | wastes CPU when events are rare |
| Interrupts | normal interactive input | needs event-loop structure |

Keyboard events are sporadic, so interrupts are usually a better fit. Polling is
still useful to understand the controller and to build small deterministic
experiments.

In the interrupt version, the program can wait for messages from the runtime and
handle keyboard input only when the controller has something to report. In the
polling version, the program repeatedly checks the status register and decides
whether to try a read. Both versions exercise the same controller rules. The
difference is how the program decides when to ask for data.

## Timeout Discipline

Polling loops need limits. A loop that waits forever for a key can hang a test,
a project demo, or a grading run.

Timer-driven limits are especially useful because they make the behavior
observable. A function that says "wait until ESC is released or until N seconds
of idle time have passed" has a precise contract. A function that says "wait for
some input" can easily become impossible to test.

> [!TIP]
> Poll with a clear maximum number of attempts or a timer-driven timeout. Then
> return an error the caller can handle.
