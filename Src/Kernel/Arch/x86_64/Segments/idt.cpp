#include "Kernel/Arch/x86_64/Cpu/Asm.h"
#include <Kernel/Arch/x86_64/Segments/Idt.h>
#include <LibFK/Log.h>

namespace idt {

void Manager::initialize() noexcept
{
    Log(LogLevel::INFO, "IDT: Initialize Interrupt Descriptor Table from x86_64 (64 Bits)");
    for (int i = 0; i < 256; ++i) {
        if (i <= 31) {
            Logf(LogLevel::TRACE, "Implement a exception for index: %d", i);
        }

        else if (i > 31 && i <= 47) {
            Logf(LogLevel::TRACE, "Implement a routine for index: %d", i);
        }

        else {
            Logf(LogLevel::TRACE, "Implement a custom handling for index %d", i);
        }
    }

    // idtr.limit = sizeof(entries_) - 1;
    // idtr.base = reinterpret_cast<LibC::uint64_t>(&entries_[0]);

    // flush_idt(&idtr);
    Log(LogLevel::INFO, "IDT: Loaded with success");
}

void Manager::set_entry(int index, void* isr, LibC::uint16_t selector, LibC::uint8_t type_attr, LibC::uint8_t ist) noexcept
{
    auto& entry = entries_[index];
    LibC::uint64_t addr = reinterpret_cast<LibC::uint64_t>(isr);

    entry.offset_low = addr & 0xFFFF;
    entry.selector = selector;
    entry.ist = ist & 0x07; // 3 bits
    entry.type_attr = type_attr;
    entry.offset_mid = (addr >> 16) & 0xFFFF;
    entry.offset_high = (addr >> 32) & 0xFFFFFFFF;
    entry.zero = 0;

    Logf(LogLevel::TRACE,
        "[IDT] Entry[%d]: isr=0x%016llx sel=0x%04x attr=0x%02x ist=%u",
        index, addr, selector, type_attr, ist);
}

}
