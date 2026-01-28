#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

class DeviceId {
public:
    DeviceId() : m_major(0), m_minor(0) {}
    DeviceId(uint16_t major, uint16_t minor) : m_major(major), m_minor(minor) {}

    uint16_t major() const { return m_major; }
    uint16_t minor() const { return m_minor; }

    bool operator==(const DeviceId& other) const {
        return m_major == other.m_major && m_minor == other.m_minor;
    }

    uint32_t raw() const { return (static_cast<uint32_t>(m_major) << 16) | m_minor; }

private:
    uint16_t m_major;
    uint16_t m_minor;
};

} // namespace fkernel
