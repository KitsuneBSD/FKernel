#pragma once
#include <LibFK/Types/types.h>

namespace fk::synchronization {

/**
 * @brief RAII wrapper to disable interrupts and restore state on destruction.
 * Uses direct x86 cli/sti/pushfq so LibFK does not depend on Kernel headers.
 */
class ScopedInterruptDisabler {
public:
    ScopedInterruptDisabler() {
        uint64_t rflags;
        asm volatile("pushfq ; popq %0" : "=r"(rflags));
        m_previous_state = (rflags & (1ULL << 9)) != 0;
        asm volatile("cli" ::: "memory");
    }

    ~ScopedInterruptDisabler() {
        if (m_previous_state) asm volatile("sti" ::: "memory");
    }

    ScopedInterruptDisabler(const ScopedInterruptDisabler&) = delete;
    ScopedInterruptDisabler& operator=(const ScopedInterruptDisabler&) = delete;

private:
    bool m_previous_state;
};

} // namespace fk::synchronization
