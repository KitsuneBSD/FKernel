#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER_WITH_ERROR_CODE(page_fault_handler, "Page Fault")
