#pragma once

#include <Kernel/Arch/x86_64/Hw/Io.h>
#include <LibC/stdint.h>

static constexpr LibC::uint8_t PIC1_CMD = 0x20;
static constexpr LibC::uint8_t PIC1_DATA = 0x21;
static constexpr LibC::uint8_t PIC2_CMD = 0xA0;
static constexpr LibC::uint8_t PIC2_DATA = 0xA1;

static constexpr LibC::uint8_t ICW1_INIT = 0x10;
static constexpr LibC::uint8_t ICW1_ICW4 = 0x01;
static constexpr LibC::uint8_t ICW4_8086 = 0x01;

class Pic8259 {
public:
    Pic8259() = delete;
    static void remap(int offset1 = 0x20, int offset2 = 0x28) noexcept;

    static void send_eoi(LibC::uint8_t irq) noexcept;

    static void mask_irq(LibC::uint8_t irq_line) noexcept;

    static void unmask_irq(LibC::uint8_t irq_line) noexcept;
};
