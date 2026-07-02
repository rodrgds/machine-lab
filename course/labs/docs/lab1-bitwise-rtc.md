# Lab 1: Bitwise Helpers And RTC

Generated folder: `labs/rtc/`

Welcome to the first Machine Lab. This lab starts with the C tools used
throughout the course: masks, shifts, fixed-width integers, output pointers,
status codes, and small helpers that can be tested without a physical device.

The device part reads the virtual CMOS/RTC through two ports:

- `RTC_ADDR_REG` (`0x70`): select the register to read.
- `RTC_DATA_REG` (`0x71`): read the value currently selected.

## Why This Matters

Device controllers rarely hand you neat C structures. They expose bytes, and
each bit may mean something different. Before you can handle timers, keyboards,
framebuffers, audio, or serial links, you need to be comfortable isolating,
setting, clearing, and combining bits.

> [!NOTE]
> The Machine Lab RTC is virtual, but the interface is shaped after the PC
> CMOS/RTC convention. The point is not the date itself. The point is learning
> how software talks to a controller through register selection and data ports.

## Device Model

The RTC exposes data registers and status registers. The most important ones
for this lab are:

| Register | Meaning |
| --- | --- |
| `RTC_SECONDS` | seconds |
| `RTC_MINUTES` | minutes |
| `RTC_HOURS` | hours |
| `RTC_DAY` | day of month |
| `RTC_MONTH` | month |
| `RTC_YEAR` | two-digit year |
| `RTC_REG_A` | status register with update-in-progress bit |
| `RTC_REG_B` | status register with data-mode bit |

The update-in-progress bit tells you whether the RTC is changing its time
registers. The data-mode bit tells you whether values are binary or BCD.

BCD means each decimal digit is stored in a nibble. For example, `0x42` means
decimal `42`, not binary `66`.

## Plan

1. Run the starter tests and watch the bitwise checks fail.
2. Implement pure bit helpers first.
3. Read RTC register B and decide whether values are BCD or binary.
4. Wait until the update-in-progress bit is clear.
5. Read date and time registers.
6. Convert values only when the RTC reports BCD mode.
7. Return a status code instead of writing partial output silently.

> [!IMPORTANT]
> Do not change RTC configuration in this lab. You should interpret the mode
> reported by the device, not force the device into the mode you prefer.

## Requested Functions

- `uint8_t bit_clear(uint8_t value, uint8_t bit)`
- `uint8_t bit_set(uint8_t value, uint8_t bit)`
- `int bit_is_set(uint8_t value, uint8_t bit)`
- `uint8_t bit_lsb(uint16_t value)`
- `uint8_t bit_msb(uint16_t value)`
- `uint8_t bit_mask(unsigned first_bit, ...)`
- `int rtc_read_date(lcom_rtc_date_t *date)`
- `int rtc_read_time(lcom_rtc_time_t *time)`

## Guided Gaps

These are guided gaps: the handout names the constraints, but the exact control
flow is yours.

- Build masks from bit positions instead of hardcoding every mask.
- Decide what should happen for out-of-range bit numbers.
- Use a sentinel to stop the variadic `bit_mask` argument list.
- Poll `RTC_REG_A` until `RTC_UIP` is clear without spinning forever.
- Keep BCD conversion small enough to test by inspection.
- Check output pointers before writing through them.
- Avoid writing a date/time structure until all required reads succeeded.

> [!TIP]
> Print register values in hexadecimal while debugging. Decimal output hides
> which nibble or bit is wrong.

## Common Mistakes

- Treating BCD as normal binary.
- Forgetting that bit positions start at zero.
- Returning a mask value from `bit_is_set` when the API asks for a boolean-like
  result.
- Reading day, month, and year while the RTC update bit is set.
- Writing to the output structure before checking all read operations.

## Discussion Prompts

- Why does `rtc_read_date` receive a pointer instead of returning a struct?
- What can go wrong if the date changes between reading day, month, and year?
- Which helper belongs in `labs/rtc/` and which belongs in `lib/rtc/`?
- How would the API change if the course wanted full four-digit years?

## Check

```sh
machinelab test rtc
machinelab run --headless --rtc 2026-06-16T12:34:56 -- build/examples/rtc_date
```

## External Reading

- GNU C bitwise operations: <https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Bitwise-Operations.html>
- GNU C shift operations: <https://www.gnu.org/software/c-intro-and-ref/manual/html_node/Shift-Operations.html>
- GNU C variadic functions: <https://www.gnu.org/software/libc/manual/html_node/Variadic-Functions.html>
- OSDev CMOS overview: <https://wiki.osdev.org/CMOS>
