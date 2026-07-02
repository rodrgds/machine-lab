# Lab 2: PIT And Timer IRQs

Generated folder: `labs/timer/`

Lab 2 introduces periodic hardware events. Instead of checking the clock by
calling a high-level OS function, you configure a timer-like device and react to
its interrupts. This is the first lab where a program must reason about time as
events rather than as a blocking delay.

The device is shaped after the PC i8254 Programmable Interval Timer. Machine
Lab exposes the same kind of concepts: timer selection, control words, divisor
values, status reads, IRQ subscription, and an interrupt handler.

## Why This Matters

Games, animations, audio buffers, input timeouts, and protocol retries all need
predictable timing. A good event loop does not sleep blindly for long periods. It
reacts to timer events and updates application state in small steps.

> [!NOTE]
> The runtime is deterministic in headless mode. That makes tests repeatable,
> but the same structure also prepares you for real hardware where timer events
> arrive asynchronously.

## Device Model

| Part | Role |
| --- | --- |
| Timer 0 | periodic interrupt source |
| Control word | selects timer, access mode, operating mode, and BCD/binary |
| Divisor | converts base clock frequency into requested rate |
| IRQ line | event path from device to program |
| Interrupt handler | small function that records that a tick happened |

You will not build a scheduler. You will build the small layer that lets later
programs count ticks and make timing decisions.

## Plan

1. Read the requested frequency.
2. Validate timer number and frequency range.
3. Compute the PIT divisor from the base frequency.
4. Preserve control bits that should not change.
5. Write low byte and high byte in the required order.
6. Subscribe to timer interrupts.
7. Keep the interrupt handler small: record a tick and return.

> [!IMPORTANT]
> Interrupt handlers should not contain large application logic. Use the handler
> to record device events, then let the main loop decide what to do.

## Requested Functions

- `int timer_set_frequency(uint8_t timer, uint32_t freq)`
- `int timer_get_conf(uint8_t timer, uint8_t *status)`
- `int timer_subscribe(lcom_irq_t *irq)`
- `int timer_unsubscribe(lcom_irq_t *irq)`
- `void timer_ih(void)`
- `uint32_t timer_ticks(void)`

## Guided Gaps

- Decide how to reject invalid timer numbers and impossible frequencies.
- Build the control word from fields rather than magic constants.
- Preserve access mode and operating mode when appropriate.
- Split a 16-bit divisor into low and high bytes.
- Track ticks without relying on wall-clock time.
- Make subscribe/unsubscribe symmetric.

> [!TIP]
> When divisor math fails, print the base frequency, requested frequency, final
> divisor, low byte, and high byte. Most timer bugs are arithmetic or byte-order
> bugs.

## Common Mistakes

- Dividing in the wrong direction.
- Accepting a divisor of zero.
- Writing only one byte of the divisor.
- Counting ticks in the main loop instead of in the interrupt handler.
- Forgetting to unsubscribe after a timed test.

## Discussion Prompts

- Why does the timer API expose `timer_ticks()` instead of sleeping directly?
- What changes if your game updates at 60 Hz but the timer fires at a different
  frequency?
- Which timing values should be constants and which should be runtime
  configuration?

## Check

```sh
machinelab test timer
machinelab run --headless -- build/examples/timer_int 3
```

## External Reading

- OSDev Programmable Interval Timer: <https://wiki.osdev.org/Programmable_Interval_Timer>
- OSDev Interrupts overview: <https://wiki.osdev.org/Interrupts>
- GNU C integer types: <https://www.gnu.org/software/libc/manual/html_node/Integers.html>
