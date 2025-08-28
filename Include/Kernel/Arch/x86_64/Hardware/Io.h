#pragma once

#include <LibC/stdint.h>

namespace Io {
constexpr LibC::uint16_t PIC1_COMMAND = 0x20;
constexpr LibC::uint16_t PIC2_COMMAND = 0xA0;
constexpr LibC::uint8_t PIC_EOI = 0x20;

void outb(LibC::uint16_t port, LibC::uint8_t val) noexcept;
LibC::uint8_t inb(LibC::uint16_t port) noexcept;

void outw(LibC::uint16_t port, LibC::uint16_t val) noexcept;
LibC::uint16_t inw(LibC::uint16_t port) noexcept;

void outl(LibC::uint16_t port, LibC::uint32_t val) noexcept;
LibC::uint32_t inl(LibC::uint16_t port) noexcept;

void send_eoi(int irq_number) noexcept;
void io_wait() noexcept;
}
