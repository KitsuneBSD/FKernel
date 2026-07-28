#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER(overflow_handler, "Overflow", SIGFPE, FPE_INTOVF)
