#pragma once

#include <Kernel/Syscall/syscall_arch.h>

#ifdef __cplusplus
extern "C" {
#endif

long syscall(long number, ...);

#ifdef __cplusplus
}
#endif
