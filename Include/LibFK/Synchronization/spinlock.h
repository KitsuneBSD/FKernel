#pragma once

#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Synchronization/lock_rank.h>
#include <LibFK/Types/types.h>

namespace fk::synchronization {

/**
 * @brief Simple atomic spinlock using xchg.
 */
class Spinlock {
public:
    constexpr Spinlock()
        : m_lock(0), m_owner_cpu(0), m_recursion_count(0), m_rank(LockRank::None) {}

    explicit constexpr Spinlock(LockRank rank)
        : m_lock(0), m_owner_cpu(0), m_recursion_count(0), m_rank(rank) {}

    void lock() {
        uint32_t cpu_id = 0;
#ifdef __fkernel__
        // Get CPU ID via local APIC ID (using assembly to avoid including APIC header)
        // This is safe even before APIC is fully initialized by the manager.
        uint32_t ebx;
        asm volatile("cpuid" : "=b"(ebx) : "a"(1) : "rcx", "rdx");
        cpu_id = (ebx >> 24) + 1;
#endif

        if (m_owner_cpu == cpu_id && cpu_id != 0) {
            m_recursion_count = m_recursion_count + 1;
            return;
        }

        // Lock rank check: must acquire locks in rank order to prevent deadlocks
        if (m_rank != LockRank::None && current_cpu_lock_rank() >= m_rank) {
            fk::algorithms::kwarn("SPINLOCK", "Lock rank violation: acquired rank %d while holding %d",
                                  (int)m_rank, (int)current_cpu_lock_rank());
        }

        while (__sync_lock_test_and_set(&m_lock, 1)) {
            while (m_lock) {
                asm volatile("pause");
            }
        }

        m_owner_cpu = cpu_id;
        m_recursion_count = 1;
        if (m_rank != LockRank::None) {
            m_prev_rank = current_cpu_lock_rank();
            set_cpu_lock_rank(m_rank);
        }
    }

    bool try_lock() {
        uint32_t cpu_id = 0;
#ifdef __fkernel__
        uint32_t ebx;
        asm volatile("cpuid" : "=b"(ebx) : "a"(1) : "rcx", "rdx");
        cpu_id = (ebx >> 24) + 1;
#endif

        if (m_owner_cpu == cpu_id && cpu_id != 0) {
            m_recursion_count = m_recursion_count + 1;
            return true;
        }

        if (__sync_lock_test_and_set(&m_lock, 1)) {
            return false;
        }

        m_owner_cpu = cpu_id;
        m_recursion_count = 1;
        return true;
    }

    void unlock() {
        uint32_t new_count = m_recursion_count - 1;
        m_recursion_count = new_count;
        if (new_count == 0) {
            if (m_rank != LockRank::None)
                set_cpu_lock_rank(m_prev_rank);
            m_owner_cpu = 0;
            __sync_lock_release(&m_lock);
        }
    }

    bool is_locked() const { return m_lock != 0; }
    LockRank rank() const { return m_rank; }

private:
    volatile int      m_lock;
    volatile uint32_t m_owner_cpu;
    volatile uint32_t m_recursion_count;
    LockRank          m_rank;
    LockRank          m_prev_rank{LockRank::None};
};

/**
 * @brief RAII wrapper for Spinlock.
 */
class ScopedLock {
public:
    explicit ScopedLock(Spinlock& lock) : m_lock(lock) {
        m_lock.lock();
    }
    ~ScopedLock() {
        m_lock.unlock();
    }

private:
    Spinlock& m_lock;
};

#ifdef __fkernel__
/**
 * @brief RAII wrapper for Spinlock that disables interrupts.
 * Uses direct x86 cli/sti/pushfq so LibFK does not depend on Kernel headers.
 */
class ScopedLockIRQ {
public:
    explicit ScopedLockIRQ(Spinlock& lock) : m_lock(lock) {
        uint64_t rflags;
        asm volatile("pushfq ; popq %0" : "=r"(rflags));
        m_interrupt_state = (rflags & (1ULL << 9)) != 0;
        asm volatile("cli" ::: "memory");
        m_lock.lock();
    }
    ~ScopedLockIRQ() {
        m_lock.unlock();
        if (m_interrupt_state) asm volatile("sti" ::: "memory");
    }

private:
    Spinlock& m_lock;
    bool m_interrupt_state;
};
#else
// Host-side stub for unit tests: no interrupts to manage, single-threaded context.
using ScopedLockIRQ = ScopedLock;
#endif

} // namespace fk::synchronization
