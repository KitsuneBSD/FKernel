#pragma once

#include <LibC/stdint.h>

class USBTransfer {
public:
    enum class Type {
        Control,
        Bulk,
        Interrupt,
        Isochronous
    };

    enum class Direction {
        In,
        Out
    };

    USBTransfer() = default;
    virtual ~USBTransfer() = default;

    Type type() const { return m_type; }
    void set_type(Type t) { m_type = t; }

    Direction direction() const { return m_direction; }
    void set_direction(Direction d) { m_direction = d; }

    uint8_t endpoint_address() const { return m_endpoint_address; }
    void set_endpoint_address(uint8_t addr) { m_endpoint_address = addr; }

    // Buffer and length would typically be here

private:
    Type m_type { Type::Control };
    Direction m_direction { Direction::In };
    uint8_t m_endpoint_address { 0 };
};
