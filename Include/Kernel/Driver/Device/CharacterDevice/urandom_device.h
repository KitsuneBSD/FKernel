#pragma once

#include <Kernel/Driver/Device/CharacterDevice/character_device.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>

namespace fkernel {

// A minimal LCG-based PRNG. Not cryptographically secure — satisfies
// programs that just need non-zero bytes (musl's arc4random_buf, etc.).
class UrandomDevice final : public CharacterDevice {
    mutable uint64_t m_state{0x123456789ABCDEF0ULL};

    uint64_t next() const {
        m_state ^= m_state << 13;
        m_state ^= m_state >> 7;
        m_state ^= m_state << 17;
        return m_state;
    }

public:
    UrandomDevice() {
        set_name("urandom");
        m_state ^= TickManager::the().get_ticks();
    }
    virtual ~UrandomDevice() override = default;

    virtual fk::core::Result<size_t, fk::core::Error> read(uint64_t, size_t size, uint8_t* buf) override {
        if (!buf) return fk::core::Error::InvalidParameter;
        size_t i = 0;
        while (i < size) {
            uint64_t v = next();
            size_t chunk = size - i;
            if (chunk > 8) chunk = 8;
            for (size_t j = 0; j < chunk; ++j)
                buf[i++] = (uint8_t)(v >> (j * 8));
        }
        return size;
    }

    virtual fk::core::Result<size_t, fk::core::Error> write(uint64_t, size_t size, const uint8_t*) override {
        return size; // Discard
    }

    virtual size_t size() const override { return 0; }
};

} // namespace fkernel
