# Bitwise Operations

Registers are compact. A single byte may contain several independent fields:

```text
Bit 7 | Bit 6 | Bit 5 | Bit 4 | Bit 3 | Bit 2 | Bit 1 | Bit 0
------+-------+-------+-------+-------+-------+-------+-------
 UIP  |  DV2  |  DV1  |  DV0  |  RS3  |  RS2  |  RS1  |  RS0
```

The `UIP` bit is bit 7. To isolate it:

```c
uint8_t uip = reg_a & 0x80;
```

To build the mask without hardcoding:

```c
uint8_t mask = (uint8_t)(1u << 7);
```

## Read Values

Use `&` to test whether a bit is present. The operation keeps only the bits
that are set in both operands, so a mask with one bit set is enough to ask a
yes-or-no question about that position. In C, the result does not need to be
exactly `1` to be true; any non-zero value is true. This is why a helper such as
`bit_is_set` should be clear about whether it returns the raw mask value or a
normalized boolean-like result.

```c
if (value & BIT(7)) {
  /* bit 7 is set */
}
```

## Construct Write Values

Use `|` to set bits and `& ~mask` to clear bits. These two operations are the
basic tools for building control words. They also make it possible to change one
field while preserving neighboring fields, which is essential when a register
contains several unrelated settings.

```c
value |= BIT(2);
value &= (uint8_t)~BIT(2);
```

This lab mostly reads the RTC. Later labs use the same idea to construct timer
control words, UART line-control values, and mouse/keyboard commands.

When you are debugging bitwise code, write the value in binary on paper or in a
comment beside the test. Many mistakes are not arithmetic mistakes; they are
position mistakes. Bit 0 is the least significant bit, so shifting by one moves
to bit 1, not to the second byte or to a decimal digit.

> [!TIP]
> Write masks from bit positions, then name the mask. Named masks document the
> device protocol and reduce off-by-one errors.
