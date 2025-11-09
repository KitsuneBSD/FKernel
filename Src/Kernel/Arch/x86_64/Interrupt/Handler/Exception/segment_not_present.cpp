#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_WITH_ERROR_CODE(segment_not_present_handler, "Segment Not Present")
