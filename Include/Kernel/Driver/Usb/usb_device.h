#pragma once

#include <LibFK/Types/types.h>

class USBDevice {
public:
    USBDevice() = default;
    virtual ~USBDevice() = default;

    uint8_t address() const { return m_address; }
    void set_address(uint8_t addr) { m_address = addr; }

    uint8_t port() const { return m_port; }
    void set_port(uint8_t port) { m_port = port; }

private:
    uint8_t m_address { 0 };
    uint8_t m_port { 0 };
};
