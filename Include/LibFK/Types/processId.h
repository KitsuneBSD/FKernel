#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/Assertions.h>

namespace fk {

class ProcessId {
public:
    static constexpr uint64_t INVALID = 0xFFFFFFFFFFFFFFFF;

    constexpr ProcessId() : m_id(INVALID) {}
    
    constexpr explicit ProcessId(uint64_t id) : m_id(id) {
        // PID 0 is Idle, so it's valid. But we check for our new INVALID constant.
        ASSERT(id < 0xFFFFFFFF || id == INVALID);
    }

    uint64_t value() const { 
        ASSERT(is_valid());
        return m_id; 
    }

    bool is_valid() const { return m_id != INVALID; }
    bool is_root() const { return m_id == 1; }
    bool is_idle() const { return m_id == 0; }

    bool operator==(const ProcessId& other) const { return m_id == other.m_id; }
    bool operator!=(const ProcessId& other) const { return m_id != other.m_id; }

private:
    uint64_t m_id;
};

} // namespace fk
