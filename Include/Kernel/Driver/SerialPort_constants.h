#pragma once

#include <LibC/stdint.h>

namespace Register {
constexpr LibC::uint8_t DATA = 0; // Transmit/Receive
constexpr LibC::uint8_t INTERRUPT_ENABLE = 1;
constexpr LibC::uint8_t FIFO_CONTROL = 2;
constexpr LibC::uint8_t LINE_CONTROL = 3;
constexpr LibC::uint8_t MODEM_CONTROL = 4;
constexpr LibC::uint8_t LINE_STATUS = 5;
}

namespace LineStatus {
constexpr LibC::uint8_t TRANSMIT_HOLD_EMPTY = 0x20;
}

namespace LineControl {
constexpr LibC::uint8_t DLAB = 0x80;
}

namespace Defaults {
constexpr LibC::uint8_t DIVISOR_LSB = 0x03; // Para 38400 baud
constexpr LibC::uint8_t DIVISOR_MSB = 0x00;
constexpr LibC::uint8_t LINE_CONTROL = 0x03;  // 8 bits, no parity, 1 stop
constexpr LibC::uint8_t FIFO_CONTROL = 0xC7;  // Enable FIFO, clear, 14-byte threshold
constexpr LibC::uint8_t MODEM_CONTROL = 0x0B; // IRQs, RTS/DSR
}
