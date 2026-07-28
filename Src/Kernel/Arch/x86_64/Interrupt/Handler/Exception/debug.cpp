#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_USER(debug_handler, "Debug Exception", SIGTRAP, TRAP_TRACE)
