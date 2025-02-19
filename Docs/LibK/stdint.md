# Stdint.md

Stdint.h is a header file in the C standard library to allow programmers to write a more portable code providing a set of: 

1. exact-width integer types
2. minimum and maximum allowable values for each types using macros.

## Integer types definition

### Fixed-Width Integer Types 

These types guarantee an exact number of bits:

#### Signed Values

- `int8_t`: Signed 8 bits.
- `int16_t`: Signed 16 bits.
- `int32_t`: Signed 32 bits.
- `int64_t`: Signed 64 bits.

#### Unsigned Values

- `uint8_t`: Unsigned 8-bit integer.
- `uint16_t`: Unsigned 16-bit integer.
- `uint32_t`: Unsigned 32-bit integer.
- `uint64_t`: Unsigned 64-bit integer.

##### Example 

```c
#include <stdint.h>

int32_t counter = 0;
uint64_t size = 1024ULL;
```

### Minimum-Width Integer Types

These types ensure at least the specified number of bits

#### Signed Values 

- `int_least8_t`: Signed integer with a minimum of 8 bits.
- `int_least16_`: Signed integer with a minimum of 16 bits.
- `int_least32_`: Signed integer with a minimum of 32 bits.
- `int_least64_`: Signed integer with a minimum of 64 bits.

#### Unsigned Values

- `uint_least8_t`: Unsigned integer with a minimum of 8 bits.
- `uint_least16_t`: Unsigned integer with a minimum of 16 bits.
- `uint_least32_t`: Unsigned integer with a minimum of 32 bits.
- `uint_least64_t`: Unsigned integer with a minimum of 64 bits.


##### Example

```c
#include <stdint.h>

int_least16_t temperature = -273;
```
### Fastest Minimum-Width Integer Width

Optimized for performance, these types are the fastest available with at least the specified width

#### Signed Values

- `int_fast8_t:` Fastest signed integer with a minimum of 8 bits.
- `int_fast16_t`: Fastest signed integer with a minimum of 16 bits.
- `int_fast32_t`: Fastest signed integer with a minimum of 32 bits.
- `int_fast64_t`: Fastest signed integer with a minimum of 64 bits.

#### Unsigned Values

- `uint_fast8_t`: Fastest unsigned integer with a minimum of 8 bits.
- `uint_fast16_`: Fastest unsigned integer with a minimum of 16 bits.
- `uint_fast32_`: Fastest unsigned integer with a minimum of 32 bits.
- `uint_fast64_`: Fastest unsigned integer with a minimum of 64 bits.

##### Example 

```c
#include <stdint.h>

uint_fast32_t index = 0;
```
## Additional Integer Types

- `intptr_t`: Signed integer type capable of holding a pointer.
- `uintptr_t`: Unsigned integer type capable of holding a pointer.
- `intmax_t`: Largest signed integer type supported.
- `uintmax_t`: Largest unsigned integer type supported.

##### Example

```c
#include <stdint.h>

uintptr_t address = (uintptr_t) &variable;
```

## Macros

To define integer constants with specific types, the following macros are provided

- `INT8_C(value)`: Defines a constant of type `int8_t`.
- `UINT8_C(value)`: Defines a constant of type `uint8_t`.
- `INT16_C(value)`: Defines a constant of type `int16_t`.
- `UINT16_C(value)`: Defines a constant of type `uint16_t`.
- `INT32_C(value)`: Defines a constant of type `int32_t`.
- `UINT32_C(value)`: Defines a constant of type `uint32_t`.
- `INT64_C(value)`: Defines a constant of type `int64_t`.
- `UINT64_C(value)`: Defines a constant of type `uint64_t`.
- `INTMAX_C(value)`: Defines a constant of type `intmax_t`.
- `UINTMAX_C(value)`: Defines a constant of type `uintmax_t`.

##### Example 

```c
#include <stdint.h>

int64_t large_number = INT64_C(9223372036854775807);
uintmax_t max_value = UINTMAX_C(18446744073709551615);
```
## Limits of the integer type

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


