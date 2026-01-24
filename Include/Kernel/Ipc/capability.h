#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

enum class CapabilityType : uint8_t {
    None = 0,
    Endpoint,
    Notification,
    SharedMemory,
};

class Capability {
    void* m_object{nullptr};
    CapabilityType m_type{CapabilityType::None};

public:
    Capability() = default;
    Capability(void* object, CapabilityType type) : m_object(object), m_type(type) {}

    void* object() const { return m_object; }
    CapabilityType type() const { return m_type; }
    bool is_valid() const { return m_type != CapabilityType::None; }
};

}
}
