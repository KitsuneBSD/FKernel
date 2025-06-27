#pragma once

#include <LibC/stdint.h>

class Pit {
public:
    Pit() = delete;

    static void initialize(LibC::uint32_t frequency) noexcept;

    static void send_command(LibC::uint8_t command) noexcept;

    static void set_divisor(LibC::uint16_t divisor) noexcept;
};
