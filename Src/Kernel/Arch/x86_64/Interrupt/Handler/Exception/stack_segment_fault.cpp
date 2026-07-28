#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER_WITH_ERROR(stack_segment_fault_handler, "Stack-Segment Fault", SIGSEGV, SEGV_MAPERR)
