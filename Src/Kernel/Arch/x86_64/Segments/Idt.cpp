#include <Kernel/Arch/x86_64/idt.h>
#include <LibFK/Log.hpp>

namespace idt {
void Manager::initialize() noexcept
{
    for (int i = 0; i < static_cast<int>(IDT_ENTRIES); i++) {
        entries_[i] = {};
    }

    idtr_.limit = sizeof(entries_) - 1;
    idtr_.base = reinterpret_cast<LibC::uint64_t>(&entries_);

    idt_flush(&idtr_);
    Log(LogLevel::INFO, "IDT Flushed with success");
}

void Manager::set_entry(int vector, void (*handler)(), LibC::uint8_t ist, LibC::uint8_t type_attr) noexcept
{
    auto addr = reinterpret_cast<LibC::uint64_t>(handler);
    entries_[vector].offset_low = static_cast<LibC::uint16_t>(addr & 0xFFFF);
    entries_[vector].selector = 0x08;
    entries_[vector].ist = ist & 0x7;
    entries_[vector].type_attr = type_attr;
    entries_[vector].offset_middle = static_cast<LibC::uint16_t>((addr >> 16) & 0xFFFF);
    entries_[vector].offset_high = static_cast<LibC::uint32_t>((addr >> 32) & 0xFFFFFFFF);
    entries_[vector].zero = 0;
}

void Manager::register_exception(int vector, void (*handler)()) noexcept
{
    constexpr LibC::uint8_t IST = 1;
    constexpr LibC::uint8_t TYPE_ATTR = 0x8E; // 1000 1110b

    set_entry(vector, handler, IST, TYPE_ATTR);
}

void Manager::register_irq(int irq_number, void (*handler)()) noexcept
{
    constexpr LibC::uint8_t IST = 0;
    constexpr LibC::uint8_t TYPE_ATTR = 0x8E;

    int vector = 32 + irq_number; // Offset padrão IRQs no IDT

    set_entry(vector, handler, IST, TYPE_ATTR);
}

void Manager::register_syscall(int vector, void (*handler)()) noexcept
{
    constexpr LibC::uint8_t IST = 0;
    constexpr LibC::uint8_t TYPE_ATTR = 0xEF; // 1110 1111b (P=1, DPL=3, Trap Gate)

    set_entry(vector, handler, IST, TYPE_ATTR);
}

}
