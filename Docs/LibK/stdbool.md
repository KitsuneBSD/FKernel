# Stdbool.h

Stdbool.h is a header file in the C standard library that provides a Boolean data type to facilitate more readable and portable code. It defines the bool type and the constants true and false.

## Boolean Type Definition

C, prior to C99, did not have a built-in Boolean type. The stdbool.h header standardizes Boolean support with the following definitions

### Boolean Type 

- `bool`: A macro representing the `_Bool` type, which stores Boolean values as `0` (false) or `1` (true).

### Boolean Constants 

- `true`: Defined as `1`, representing a logical true value.

- `false`: Defined as `0`, representing a logical false value.

##### Example 

```c
#include <stdbool.h>
#include <stdio.h>

int main() {
    bool is_valid = true;
    if (is_valid) {
        printf("The value is true!\n");
    }
    return 0;
}
```
