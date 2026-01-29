#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Assertions.h>

namespace fk {

class ProcessId {
public:
    constexpr ProcessId() : m_id(0) {}
    constexpr explicit ProcessId(uint64_t id) : m_id(id) {
        // PID 0 is allowed (Idle Task), but we can restrict max value
        ASSERT(id < 0xFFFFFFFF);
    }

    uint64_t value() const { return m_id; }
    bool is_valid() const { return m_id != 0; }

    bool operator==(const ProcessId& other) const { return m_id == other.m_id; }
    bool operator!=(const ProcessId& other) const { return m_id != other.m_id; }

private:
    uint64_t m_id;
};

} // namespace fk
