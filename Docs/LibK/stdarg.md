# stdarg.h

stdarg.h is a header file in the C standard library that provides facilities for stepping through a list of function arguments whose number and types are not known to the called function. In a freestanding environment, where we avoid relying on a complete C runtime library, a manual implementation is necessary.

## Variadic Argument Handling

### Core Definitions

The implementation provides macros to manage variadic arguments:

- `va_list`: A type to hold the information needed by `va_start`, `va_arg`, and `va_end`.
- `va_start(ap, last)`: Initializes ap to point to the first variadic argument.
- `va_arg(ap, type)`: Retrieves the next argument of the given type from the variadic argument list.
- `va_end(ap)`: Cleans up; in our implementation, this simply zeroes the pointer.
- `_INTSIZEOF(n)`: A helper macro to ensure correct alignment for accessing the arguments.

##### Example

```c
#include "stdarg.h"

int sum(int count, ...) {
    int total = 0;
    va_list ap;
    va_start(ap, count);

    for (int i = 0; i < count; i++) {
        total += va_arg(ap, int);
    }

    va_end(ap);
    return total;
}

int main(void) {
    int result = sum(4, 10, 20, 30, 40);
    // Expected output: 100
    return 0;
}
```
