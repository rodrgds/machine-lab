# Lab 1: Minix-Style I/O Without The VM

Generated folder: `labs/rtc/`

Lab 1 introduces the mental model used for the rest of the course. A program
does not begin by calling a convenient operating-system service. Instead, it
talks to a small controller through registers, and those registers are only
bytes until the programmer understands which bits have meaning.

The concrete device is the RTC/CMOS clock. It is intentionally modest: the
device has a tiny register set, the protocol is easy to observe, and the errors
are understandable. Even so, it already contains most of the habits that matter
later: checking status before trusting data, decoding values according to a
configuration bit, returning errors separately from output values, and writing C
helpers that are small enough to inspect.

In the original Minix setting, a lab like this also taught how a user-space
service could receive permission to perform privileged I/O. Machine Lab keeps
the same controller-facing work, but moves the hardware behind a safe runtime so
that the first exercise can focus on the protocol itself.

## What You Will Learn

By the end of this lab you should be able to explain why older PC devices used
I/O ports, how to isolate and construct bit fields, why a controller exposes
status before data, how BCD differs from a normal binary integer, and why many C
APIs return a status code while filling caller-owned structures through output
pointers.

## Plan

Start by reading [why I/O looked this way](/labs/lab1/context), then study the
[port pair used by the RTC](/labs/lab1/io-ports). Once the controller shape is
clear, implement the [bitwise helpers](/labs/lab1/bitwise) before reading
[date and time from the RTC](/labs/lab1/rtc). The [implementation tasks](/labs/lab1/tasks)
collect the required functions, and the [checks](/labs/lab1/check) show how to
validate your work. Finish by comparing the old interface with
[modern hardware access](/labs/lab1/modern), because the bus changes over time
but the protocol discipline does not.

> [!TIP]
> Do the bit helpers first. They are pure C and give you confidence before you
> add controller reads.
