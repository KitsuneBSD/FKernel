#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_WITH_ERROR_CODE(double_fault_handler, "Double Fault")
