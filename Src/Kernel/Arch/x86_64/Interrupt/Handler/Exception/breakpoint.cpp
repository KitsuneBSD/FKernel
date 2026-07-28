#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER(breakpoint_handler, "Breakpoint", SIGTRAP, TRAP_BRKPT)
