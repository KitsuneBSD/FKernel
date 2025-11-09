#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>

GENERIC_EXCEPTION_HANDLER(virtualization_exception_handler,
                          "Virtualization Exception")
