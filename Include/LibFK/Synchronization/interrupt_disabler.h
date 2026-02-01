#pragma once

#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>

namespace fk::synchronization {

/**
 * @brief RAII wrapper to disable interrupts and restore state on destruction.
 */
class ScopedInterruptDisabler {
public:
    ScopedInterruptDisabler() {
        m_previous_state = InterruptController::the().get_interrupt_state();
        InterruptController::the().disable_interrupt();
    }

    ~ScopedInterruptDisabler() {
        if (m_previous_state) {
            InterruptController::the().enable_interrupt();
        }
    }

    // Explicitly delete copy constructor and assignment operator
    ScopedInterruptDisabler(const ScopedInterruptDisabler&) = delete;
    ScopedInterruptDisabler& operator=(const ScopedInterruptDisabler&) = delete;

private:
    bool m_previous_state;
};

} // namespace fk::synchronization
