#pragma once
#include <LibFK/Arch/cpu.h>
#include <LibFK/Types/types.h>

namespace fk::synchronization {

class ScopedInterruptDisabler {
public:
    ScopedInterruptDisabler() {
        m_previous_state = fk::arch::save_and_disable_interrupts();
    }

    ~ScopedInterruptDisabler() {
        fk::arch::restore_interrupts(m_previous_state);
    }

    ScopedInterruptDisabler(const ScopedInterruptDisabler&) = delete;
    ScopedInterruptDisabler& operator=(const ScopedInterruptDisabler&) = delete;

private:
    bool m_previous_state;
};

} // namespace fk::synchronization
