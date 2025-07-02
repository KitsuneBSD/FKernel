#include "Kernel/Arch/x86_64/Interrupts/Routines.h"
#include "LibFK/log.h"
#include <Kernel/Arch/x86_64/Cpu/State.h>
#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>

inline void halt()
{
    while (true) {
        asm("hlt");
    }
}

extern "C" void global_default_handler(CpuState* const frame)
{

    Logf(LogLevel::ERROR, "%u: Exception: %s isn't allowed", uptime(), named_exception(frame->error_code));
    halt();
}
