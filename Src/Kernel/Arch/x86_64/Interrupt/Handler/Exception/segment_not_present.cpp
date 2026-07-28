#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER_WITH_ERROR(segment_not_present_handler, "Segment Not Present", SIGBUS, BUS_ADRERR)
