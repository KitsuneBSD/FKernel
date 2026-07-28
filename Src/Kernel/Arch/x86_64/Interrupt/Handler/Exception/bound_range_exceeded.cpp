#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER(bound_range_exceeded_handler, "Bound Range Exceeded", SIGSEGV, SEGV_MAPERR)
