#pragma once

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

// Global rank tracker (single-CPU; per-CPU slot tracking is a future TODO).
// Lives here as an inline variable so no separate .cpp is needed.
inline LockRank g_current_lock_rank = LockRank::None;

// Returns the highest rank currently held by the calling CPU.
inline LockRank current_cpu_lock_rank() {
    return g_current_lock_rank;
}

inline void set_cpu_lock_rank(LockRank rank) {
    g_current_lock_rank = rank;
}

} // namespace fk::synchronization
