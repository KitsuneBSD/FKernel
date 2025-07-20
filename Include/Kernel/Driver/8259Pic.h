#pragma once

#include <LibC/stdint.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#    include <Kernel/Arch/x86_64/Hardware/Io.h>
#    include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>
#endif

class Pic8259 {
public:
    Pic8259(Pic8259 const&) = delete;
    Pic8259& operator=(Pic8259 const&) = delete;

    static Pic8259& Instance() noexcept
    {
        static Pic8259 instance;
        return instance;
    }

    void remap(int master_offset, int slave_offset) noexcept;

    void send_eoi(LibC::uint8_t irq) noexcept;

    void mask_irq(LibC::uint8_t irq_line) noexcept;

    void unmask_irq(LibC::uint8_t irq_line) noexcept;

private:
    Pic8259() = default;

    void validate_irq_line(LibC::uint8_t irq_line) const noexcept;

    void send_eoi_master() noexcept;
    void send_eoi_slave() noexcept;

    LibC::uint16_t get_port_for_irq(LibC::uint8_t irq_line) const noexcept;
    LibC::uint8_t get_irq_mask_bit(LibC::uint8_t irq_line) const noexcept;
};
