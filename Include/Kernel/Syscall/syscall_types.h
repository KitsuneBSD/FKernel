#pragma once

#include <LibFK/Types/types.h>

#ifdef __x86_64__
struct PtRegs;
typedef uint64_t (*syscall_function_t)(uint64_t, uint64_t, uint64_t, uint64_t,
                                       uint64_t, uint64_t, PtRegs*);
#else
typedef uint64_t (*syscall_function_t)(uint64_t, uint64_t, uint64_t, uint64_t,
                                       uint64_t, uint64_t, void*);
#endif
