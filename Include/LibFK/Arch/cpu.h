#pragma once

// Arch-portable CPU primitive dispatcher.
// Kernel builds (__fkernel__) get real x86_64 implementations.
// Host/test builds get no-op stubs so unit tests compile without asm.

#ifdef __fkernel__
#include <LibFK/Arch/x86_64/cpu_primitives.h>
namespace fk {
namespace arch {
    inline void     cpu_relax()                           { x86_64::cpu_relax(); }
    inline void     cpu_halt_forever()                    { x86_64::cpu_halt_forever(); }
    inline bool     save_and_disable_interrupts()         { return x86_64::save_and_disable_interrupts(); }
    inline void     restore_interrupts(bool was_enabled)  { x86_64::restore_interrupts(was_enabled); }
    inline uint32_t get_apic_id()                         { return x86_64::get_apic_id(); }
} // namespace arch
} // namespace fk
#else
#include <LibFK/Types/types.h>
namespace fk {
namespace arch {
    inline void     cpu_relax() {}
    [[noreturn]] inline void cpu_halt_forever() { while (true) {} }
    inline bool     save_and_disable_interrupts()        { return false; }
    inline void     restore_interrupts(bool) {}
    inline uint32_t get_apic_id()                        { return 0; }
} // namespace arch
} // namespace fk
#endif
