#pragma once

#include <LibFK/Types/types.h>

#ifdef __fkernel__
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#endif

namespace fk::synchronization {

/**
 * @brief Simple atomic spinlock using xchg.
 */
class Spinlock {
public:
    constexpr Spinlock() : m_lock(0), m_owner_cpu(0), m_recursion_count(0) {}

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

        while (__sync_lock_test_and_set(&m_lock, 1)) {
            while (m_lock) {
                asm volatile("pause");
            }
        }
        
        m_owner_cpu = cpu_id;
        m_recursion_count = 1;
    }

    void unlock() {
        uint32_t new_count = m_recursion_count - 1;
        m_recursion_count = new_count;
        if (new_count == 0) {
            m_owner_cpu = 0;
            __sync_lock_release(&m_lock);
        }
    }

    bool is_locked() const { return m_lock != 0; }

private:
    volatile int m_lock;
    volatile uint32_t m_owner_cpu;
    volatile uint32_t m_recursion_count;
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
 */
class ScopedLockIRQ {
public:
    explicit ScopedLockIRQ(Spinlock& lock) : m_lock(lock) {
        m_interrupt_state = InterruptController::the().get_interrupt_state();
        InterruptController::the().disable_interrupt();
        m_lock.lock();
    }
    ~ScopedLockIRQ() {
        m_lock.unlock();
        if (m_interrupt_state) {
            InterruptController::the().enable_interrupt();
        }
    }

private:
    Spinlock& m_lock;
    bool m_interrupt_state;
};
#endif

} // namespace fk::synchronization
