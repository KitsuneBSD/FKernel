#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_WITH_ERROR_CODE(stack_segment_fault_handler, "Stack-Segment Fault")
