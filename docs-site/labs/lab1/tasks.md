# Lab 1 Tasks

Work in `labs/rtc/`.

The skeleton deliberately leaves small but important pieces unfinished. You are
not expected to discover the RTC from scratch, but you are expected to turn the
protocol described in the previous pages into clear C code. Keep the helper
functions boring. If a function only needs to build a mask, it should build a
mask and return it; do not hide controller reads or formatting behavior inside
it.

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

The bit helpers should build masks from bit positions and should behave
predictably when a caller asks for an out-of-range bit. The variadic
`bit_mask` helper needs a sentinel so that it knows where the argument list
ends. The RTC helpers should poll `RTC_REG_A` until `RTC_UIP` is clear, keep BCD
conversion small and inspectable, and check output pointers before writing.

You will also need to decide which helpers are merely part of the exercise and
which ones deserve to become reusable library code. A `bcd_to_binary` helper is
likely to remain close to the RTC implementation. Generic bit helpers may be
useful across several labs, but only if their names and behavior are precise
enough for other students to trust.

> [!IMPORTANT]
> Do not change RTC configuration. Interpret the mode reported by the device
> instead of forcing the device into the mode you prefer.

## Common Mistakes

The most common failures in this lab come from treating BCD as binary,
forgetting that bit positions start at zero, returning a mask value when the API
asks for boolean-like output, or writing a partial date/time structure after one
read failed. These bugs are small enough that tests should lead you directly to
them, but only if you keep each helper focused.

Next: [checks and references](/labs/lab1/check).
