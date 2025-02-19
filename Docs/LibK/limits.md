# Limits.h

The header defines macros specifying the minimum and maximum values for each integer types

### Limits of Integers

#### Signed Values
- `INT8_MIN` to `INT64_MIN`: Minimum values for signed integer types.
- `INT8_MAX` to `INT64_MAX`: Maximum values for signed integer types.
- `INT_LEAST8_MIN` to `INT_LEAST64_MIN`: Minimum values for least-width signed integer types.
- `INT_LEAST8_MAX` to `INT_LEAST64_MAX`: Maximum values for least-width signed integer types.
- `INT_FAST8_MIN` to `INT_FAST64_MIN`: Minimum values for fastest minimum-width signed integer types.
- `INT_FAST8_MAX` to `INT_FAST64_MAX`: Maximum values for fastest minimum-width signed integer types.

#### Unsigned Values
- `UINT8_MAX` to `UINT64_MAX`: Maximum values for unsigned integer types.
- `UINT_LEAST8_MAX` to `UINT_LEAST64_MAX`: Maximum values for least-width unsigned integer types.
- `UINT_FAST8_MAX` to `UINT_FAST64_MAX`: Maximum values for fastest minimum-width unsigned integer types. 

### Another Values

- `INTPTR_MIN` and `INTPTR_MAX`: Minimum and maximum values for `intptr_t`.
- `UINTPTR_MAX`: Maximum value for `uintptr_t`.
- `INTMAX_MIN` and `INTMAX_MAX`: Minimum and maximum values for `intmax_t`.
- `UINTMAX_MAX`: Maximum value for `uintmax_t`.

##### Example 

```c
#include <stdint.h>

if (value > INT32_MAX) {
    // Handle overflow
}
```


