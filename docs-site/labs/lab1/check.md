# Lab 1 Check

After implementing the functions, run the focused RTC test first. It is faster
than launching a full example and it gives you a direct signal about the helper
contracts. Once that passes, run the headless example with a fixed virtual RTC
time so that your output is deterministic.

```sh
machinelab test rtc
machinelab run --headless --rtc 2026-06-16T12:34:56 -- build/examples/rtc_date
```

## Discussion Prompts

When reviewing the lab, make sure you can explain why `rtc_read_date` receives a
pointer instead of returning a struct, what can go wrong if the date changes
between reading day, month, and year, and which helpers belong in `labs/rtc/`
rather than a reusable `lib/rtc/` module. These questions matter because they
turn a passing test into a reusable design habit.

## External Reading

The LCOM [bitwise chapter](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab1/protocols-bitwise.html)
and [RTC chapter](https://pages.up.pt/~up748353/classes/lcom/lab-guides/lab1/rtc.html)
are useful companion readings. For C details, review the GNU C notes on
[bitwise operations](https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Bitwise-Operations.html),
[shift operations](https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Shift-Operations.html),
and [variadic functions](https://www.gnu.org/software/libc/manual/html_node/Variadic-Functions.html).
For a broader operating-system view of the device, read the
[OSDev CMOS overview](https://wiki.osdev.org/CMOS).
