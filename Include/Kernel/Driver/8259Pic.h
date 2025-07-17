#pragma once

#include <Kernel/Arch/x86_64/Hardware/Io.h>
#include <LibC/stdint.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif // DEBUG

class Pic8259 {
public:
    Pic8259() = delete;
    static void remap(int offset1 = PIC1_CMD, int offset2 = 0x28) noexcept;

    static void send_eoi(LibC::uint8_t irq) noexcept;

    static void mask_irq(LibC::uint8_t irq_line) noexcept;

    static void unmask_irq(LibC::uint8_t irq_line) noexcept;
};
