#pragma once

#include <LibC/stdint.h>

class Pit {
public:
    static Pit& Instance() noexcept;

    void initialize(LibC::uint32_t frequency) noexcept;
    void send_command(LibC::uint8_t command) noexcept;
    void set_divisor(LibC::uint16_t divisor) noexcept;

private:
    Pit() = default;

    LibC::uint16_t compute_divisor(LibC::uint32_t frequency) const noexcept;
};
