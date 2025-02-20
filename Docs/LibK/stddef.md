# Stddef.h

stddef.h is a header file in the C standard library that provides fundamental definitions for memory and object management, making it essential for low-level programming and system development.

## Type Definitions

- `size_t`

A type used to represent sizes of objects in memory. It is an unsigned integer type.

```c
#include <stddef.h>
size_t length = 10;
```

- `ptrdiff_t`

A signed integer type used to store the result of pointer subtraction

```c
#include <stddef.h>
int array[10];
ptrdiff_t difference = &array[5] - &array[2];
```

- `wchar_t`

A type for representing wide characters. It is typically used for Unicode character storage.

```c
#include <stddef.h>
wchar_t ch = L'A';
```

## Macros

- `NULL`
  A macro representing a null pointer

```c
#include <stddef.h>
int *ptr = NULL;
```

- `offsetof(type,member)`

A macro that calculates the byte offset of a member within a structure.

```c
#include <stddef.h>
struct Example {
    int a;
    double b;
};
size_t offset = offsetof(struct Example, b);
```
