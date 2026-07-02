# The i8254 Timer Controller

The i8254 PIT has several counters. Lab 2 focuses on Timer 0, the periodic
interrupt source.

| Field | Purpose |
| --- | --- |
| timer select | choose which counter is programmed |
| access mode | decide low byte, high byte, or both |
| operating mode | choose counting behavior |
| BCD/binary | choose encoding |
| divisor | convert base frequency into output frequency |

The base frequency is divided by the programmed divisor. A lower divisor means
more frequent interrupts.

This makes the timer a good example of hardware that is configured indirectly.
You do not write the desired frequency into the device. You compute the divisor
that will make the device produce approximately that frequency. The controller
then keeps counting independently from your program until it is reconfigured.

## Control Words

A control word is a byte made from fields. This is why Lab 1 matters: you will
construct a value by shifting and OR-ing fields, not by guessing a decimal
constant.

When you read examples online, you may see control words written as fixed
hexadecimal constants. That is acceptable for a narrow demo, but it is not a
good learning strategy. In this lab, build the byte from named fields. The code
should show which timer is selected, which access mode is used, which operating
mode is requested, and whether BCD is enabled.

> [!IMPORTANT]
> Timer programming has order. If the access mode says "low byte then high
> byte", write the low byte first.
