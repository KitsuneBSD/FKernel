#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace ipc {

/// @brief Metadata about an IPC message, designed to fit in a single register.
/// Layout: [Label (48 bits) | Length (12 bits) | Flags (4 bits)]
class MessageInfo {
    uint64_t m_raw;

public:
    explicit MessageInfo(uint64_t raw) : m_raw(raw) {}

    static MessageInfo create(uint64_t label, uint16_t length, uint8_t flags) {
        return MessageInfo((label << 16) | ((uint64_t)(length & 0xFFF) << 4) | (flags & 0xF));
    }

    uint64_t label() const { return m_raw >> 16; }
    uint16_t length() const { return (m_raw >> 4) & 0xFFF; }
    uint8_t flags() const { return m_raw & 0xF; }
    uint64_t raw() const { return m_raw; }
};

}
}
