# Lab 2 Tasks

Work in `labs/timer/`. This lab is the first one where the code you write is
expected to become infrastructure for later labs, so treat naming and state
ownership seriously. A later keyboard or graphics exercise should be able to
use your timer helpers without copying test-specific code.

## Requested Functions

- `int timer_set_frequency(uint8_t timer, uint32_t freq)`
- `int timer_get_conf(uint8_t timer, uint8_t *status)`
- `int timer_subscribe(lcom_irq_t *irq)`
- `int timer_unsubscribe(lcom_irq_t *irq)`
- `void timer_ih(void)`
- `uint32_t timer_ticks(void)`

## Guided Gaps

Reject invalid timer numbers and impossible frequencies before writing the
control word. Build the control word from named fields, split the 16-bit divisor
into low and high bytes, and track ticks without relying on wall-clock time.
Subscribe and unsubscribe should be symmetric: a successful setup should have a
clear cleanup path, and cleanup should not depend on unrelated application
state.

Once the required functions pass, look at them again as future project code.
The tick counter, elapsed-time helpers, and subscription wrappers probably
belong in `lib/timer/`. Lab-output formatting and one-off test harness behavior
should stay outside the library.

> [!IMPORTANT]
> Interrupt handlers should not contain large application logic. Record the
> device event, then let the main loop decide what to do.

## Common Mistakes

Typical failures include dividing in the wrong direction, accepting a divisor of
zero, writing only one byte of the divisor, and counting ticks in the main loop
instead of in the handler. If the tests report a timing mismatch, inspect the
computed divisor and byte order before rewriting the whole function.

Next: [checks and references](/labs/lab2/check).
