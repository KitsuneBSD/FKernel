#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace arch {
namespace x86_64 {

inline void cpu_relax() {
    asm volatile("pause" ::: "memory");
}

// Returns the APIC ID of the current logical CPU (bits 31:24 of CPUID.1.EBX).
inline uint32_t get_apic_id() {
    uint32_t ebx;
    asm volatile("cpuid" : "=b"(ebx) : "a"(1) : "ecx", "edx");
    return ebx >> 24;
}

inline void cpu_halt_forever() {
    while (true) {
        asm volatile("cli; hlt");
    }
}

// Saves interrupt-enable state, disables interrupts, returns previous state.
inline bool save_and_disable_interrupts() {
    uint64_t rflags;
    asm volatile("pushfq ; popq %0" : "=r"(rflags));
    bool was_enabled = (rflags & (1ULL << 9)) != 0;
    asm volatile("cli" ::: "memory");
    return was_enabled;
}

inline void restore_interrupts(bool was_enabled) {
    if (was_enabled)
        asm volatile("sti" ::: "memory");
}

} // namespace x86_64
} // namespace arch
} // namespace fk
