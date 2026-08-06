#pragma once

#include <LibFK/Arch/cpu.h>
#include <LibFK/Types/types.h>

namespace fk::synchronization {

// Ranks define allowed lock acquisition order.
// A thread holding lock at rank N may only acquire locks at rank > N.
// Violations are caught in debug builds via ASSERT.
enum class LockRank : uint32_t {
    None        = 0,   // unranked — use for locks not yet assigned a rank
    Scheduler   = 10,
    Memory      = 20,
    VFS         = 30,
    Process     = 40,
    Ipc         = 50,
    Driver      = 60,
    Network     = 70,
    Max         = 0xFFFFFFFFu,
};

// Must match or exceed the kernel's MAX_CPUS (currently 64).
// Slot 0 serves pre-APIC paths and host-side unit tests.
static constexpr uint32_t FK_MAX_CPUS = 64;

// Per-CPU rank table. Indexed by cpu_lock_slot() below.
inline LockRank g_cpu_lock_ranks[FK_MAX_CPUS] = {};

// Returns the APIC-ID-based per-CPU slot (1-indexed in kernel; 0 for host tests
// and pre-APIC kernel paths). Saturates to slot 0 on unexpected APIC ID.
inline uint32_t cpu_lock_slot() {
#ifdef __fkernel__
    uint32_t slot = fk::arch::get_apic_id() + 1;
    return (slot < FK_MAX_CPUS) ? slot : 0;
#else
    return 0;
#endif
}

inline LockRank current_cpu_lock_rank() {
    return g_cpu_lock_ranks[cpu_lock_slot()];
}

inline void set_cpu_lock_rank(LockRank rank) {
    g_cpu_lock_ranks[cpu_lock_slot()] = rank;
}

} // namespace fk::synchronization
