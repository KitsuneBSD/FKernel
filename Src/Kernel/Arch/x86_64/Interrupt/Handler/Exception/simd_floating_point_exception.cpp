#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER(simd_floating_point_exception_handler, "SIMD Floating-Point Exception", SIGFPE, FPE_FLTDIV)
