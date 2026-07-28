#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER(x87_fpu_floating_point_error_handler, "x87 FPU Floating-Point Error", SIGFPE, FPE_FLTDIV)
