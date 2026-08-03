#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

class Badge {
    uint64_t m_value;

public:
    explicit Badge(uint64_t value) : m_value(value) {}
    uint64_t value() const { return m_value; }
    bool is_empty() const { return m_value == 0; }
};

}
}
