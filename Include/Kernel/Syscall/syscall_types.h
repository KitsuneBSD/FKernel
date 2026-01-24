#pragma once

#include <LibFK/Types/types.h>

typedef uint64_t (*syscall_function_t)(uint64_t, uint64_t, uint64_t, uint64_t,
                                       uint64_t, uint64_t);
