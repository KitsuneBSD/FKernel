#pragma once

#include <LibFK/Types/types.h>

namespace fk::synchronization {

/**
 * @brief Simple atomic spinlock using xchg.
 */
class Spinlock {
public:
    constexpr Spinlock() : m_lock(0) {}

    void lock() {
        while (__sync_lock_test_and_set(&m_lock, 1)) {
            while (m_lock) {
                asm volatile("pause");
            }
        }
    }

    void unlock() {
        __sync_lock_release(&m_lock);
    }

    bool is_locked() const { return m_lock != 0; }

private:
    volatile int m_lock;
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

} // namespace fk::synchronization
