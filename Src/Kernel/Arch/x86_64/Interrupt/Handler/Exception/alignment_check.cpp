#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER_WITH_ERROR(alignment_check_handler, "Alignment Check", SIGBUS, BUS_ADRALN)
