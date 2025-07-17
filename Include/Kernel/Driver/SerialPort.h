#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace Serial {

enum class Port : LibC::uint16_t {
    COM1 = 0x3F8,
    COM2 = 0x2F8,
    COM3 = 0x3E8,
    COM4 = 0x2E8
};

class SerialPort {
private:
    LibC::uint16_t base_;
    explicit SerialPort(Port port = Port::COM1);

    bool is_transmit_ready() const;

    SerialPort(SerialPort const&) = delete;
    SerialPort& operator=(SerialPort const&) = delete;

public:
    static SerialPort& Instance();

    void initialize() const;
    void write_char(char c) const;
    void write(char const* str) const;
};

}
