#pragma once

#include <stdint.h>

// Host-side interrupt controller mock for kernel unit tests.
// All operations are no-ops; tracks enable/mask state for assertion.

namespace test_mocks {

class MockInterruptController {
public:
    static MockInterruptController& the() {
        static MockInterruptController inst;
        return inst;
    }

    void reset() {
        for (int i = 0; i < 256; ++i) {
            m_masked[i] = false;
            m_handler_count[i] = 0;
        }
        m_eoi_count = 0;
    }

    void mask_irq(uint8_t irq)   { m_masked[irq] = true; }
    void unmask_irq(uint8_t irq) { m_masked[irq] = false; }
    void send_eoi(uint8_t)       { ++m_eoi_count; }
    void register_handler(uint8_t irq) { ++m_handler_count[irq]; }

    bool is_masked(uint8_t irq) const { return m_masked[irq]; }
    uint32_t eoi_count() const { return m_eoi_count; }
    uint32_t handler_count(uint8_t irq) const { return m_handler_count[irq]; }

private:
    bool     m_masked[256]{};
    uint32_t m_handler_count[256]{};
    uint32_t m_eoi_count{0};
};

} // namespace test_mocks
