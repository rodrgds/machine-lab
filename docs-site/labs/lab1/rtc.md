# Reading The RTC

The RTC exposes data registers and status registers. The data registers contain
the current time fields. The status registers tell you how those fields should
be read and interpreted.

| Register | Meaning |
| --- | --- |
| `RTC_SECONDS` | seconds |
| `RTC_MINUTES` | minutes |
| `RTC_HOURS` | hours |
| `RTC_DAY` | day of month |
| `RTC_MONTH` | month |
| `RTC_YEAR` | two-digit year |
| `RTC_REG_A` | update-in-progress bit |
| `RTC_REG_B` | data-mode bit |

## Update-In-Progress

`RTC_REG_A` contains the `UIP` bit. When it is set, the RTC is updating its
fields. Reading during that interval can produce inconsistent data.

The safe sequence is to read `RTC_REG_A`, wait while `RTC_UIP` is set, read
`RTC_REG_B`, then read the date and time fields. Only after that should you
convert from BCD if register B says the device is in BCD mode. This sounds
slower than simply reading the day, month, and year, but correctness matters
more than shaving a few port operations from a clock read.

In a real system you may also see double-read strategies, where software reads
the full time twice and accepts it only if both reads agree. The exact strategy
depends on the hardware specification and the guarantees it provides. For this
lab, checking `UIP` is enough to teach the status-before-data habit.

## BCD

BCD stores decimal digits in nibbles:

| Raw value | Meaning |
| --- | --- |
| `0x09` | 9 |
| `0x16` | 16 |
| `0x42` | 42 |

Binary `0x42` is decimal `66`, so the conversion must be deliberate.

The point of BCD is historical compatibility with decimal displays and firmware
interfaces. It is not a better integer representation for arithmetic. Once your
program has read a BCD value, convert it into an ordinary binary integer before
the rest of the code uses it.

## Output Structures

The lab functions receive output pointers:

```c
int rtc_read_date(lcom_rtc_date_t *date);
```

This lets the function return a status code while filling the caller's
structure only on success.

That design choice is common in C systems code. Returning an `int` gives the
function a place to report failure, while the pointer gives it a place to write
structured output. Your implementation should therefore validate the pointer
before writing through it, and it should avoid leaving a half-updated structure
after a failed read.
