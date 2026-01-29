#pragma once

#include <LibFK/Types/types.h>

namespace fk {

class ProcessId {
public:
    constexpr ProcessId() : m_id(0) {}
    constexpr explicit ProcessId(uint64_t id) : m_id(id) {}

    uint64_t value() const { return m_id; }
    bool is_valid() const { return m_id != 0; }

    bool operator==(const ProcessId& other) const { return m_id == other.m_id; }
    bool operator!=(const ProcessId& other) const { return m_id != other.m_id; }

private:
    uint64_t m_id;
};

} // namespace fk
