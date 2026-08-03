#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class ProcessId {
public:
    static constexpr uint64_t INVALID = 0xFFFFFFFFFFFFFFFF;
    static constexpr uint64_t ANY = 0xFFFFFFFFFFFFFFFE; // For wait4(-1)

    constexpr ProcessId() : m_id(INVALID) {}
    
    constexpr explicit ProcessId(uint64_t id)
        : m_id(id < 0x80000000 || id == INVALID || id == ANY ? id : INVALID) {}

    static ProcessId from_signed(int64_t id) {
        if (id == -1) return ProcessId(ANY);
        if (id < 0) return ProcessId(INVALID);
        return ProcessId(static_cast<uint64_t>(id));
    }

    uint64_t value() const { 
        return m_id; 
    }

    bool is_valid() const { return m_id != INVALID; }
    bool is_any() const { return m_id == ANY; }
    bool is_root() const { return m_id == 1; }
    bool is_idle() const { return m_id == 0; }

    bool operator==(const ProcessId& other) const { return m_id == other.m_id; }
    bool operator!=(const ProcessId& other) const { return m_id != other.m_id; }

private:
    uint64_t m_id;
};

} // namespace fk
