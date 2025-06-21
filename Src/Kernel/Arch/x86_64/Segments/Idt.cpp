#include <Kernel/Arch/x86_64/exception.h>
#include <Kernel/Arch/x86_64/idt.h>
#include <Kernel/Arch/x86_64/irq_dispatch.h>
#include <Kernel/Arch/x86_64/isr.h>
#include <LibFK/Log.hpp>

namespace idt {

constexpr LibC::uint8_t IST_NONE = 0;
constexpr LibC::uint8_t IST_DOUBLE_FAULT = 1;

extern "C" void (*const irq_stubs[16])() = {
    irq0_handler, irq1_handler, irq2_handler, irq3_handler,
    irq4_handler, irq5_handler, irq6_handler, irq7_handler,
    irq8_handler, irq9_handler, irq10_handler, irq11_handler,
    irq12_handler, irq13_handler, irq14_handler, irq15_handler
};

static void (*isr_stubs[32])() = {
    isr_divide_by_zero,
    isr_debug,
    isr_nmi,
    isr_breakpoint,
    isr_overflow,
    isr_bound_range,
    isr_invalid_opcode,
    isr_device_na,
    isr_double_fault,
    isr_coprocessor_seg,
    isr_invalid_tss,
    isr_seg_not_present,
    isr_stack_fault,
    isr_gp_fault,
    isr_page_fault,
    isr_reserved_15,
    isr_fpu_error,
    isr_alignment_check,
    isr_machine_check,
    isr_simd_fp,
    isr_virtualization,
    isr_reserved_21,
    isr_reserved_22,
    isr_reserved_23,
    isr_reserved_24,
    isr_reserved_25,
    isr_reserved_26,
    isr_reserved_27,
    isr_reserved_28,
    isr_reserved_29,
    isr_reserved_30,
    isr_reserved_31
};

LibC::uint8_t const isr_ist[32] = {
    IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_DOUBLE_FAULT, // 8
    IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE
};

void Manager::initialize() noexcept
{
    for (int i = 0; i < static_cast<int>(IDT_ENTRIES); i++) {
        entries_[i] = {};
    }

    for (int i = 0; i < 32; ++i) {
        register_exception(i, isr_stubs[i]);
    }

    for (int i = 32; i < 48; ++i) {
        register_irq(i, irq_stubs[i]);
    }

    associate_irq(0, routine::timer_handler);
    associate_irq(1, routine::keyboard_handler);
    associate_irq(2, routine::cascade_handler);
    associate_irq(3, routine::com2_handler);
    associate_irq(4, routine::com1_handler);
    associate_irq(5, routine::legacy_peripheral_handler);

    associate_irq(6, routine::fdc_handler);
    associate_irq(7, routine::spurious_irq7_handler);
    associate_irq(8, routine::rtc_handler);
    associate_irq(9, routine::acpi_handler);
    associate_irq(10, routine::irq10_pci_handler);
    associate_irq(11, routine::irq11_pci_handler);
    associate_irq(12, routine::ps2_mouse_handler);
    associate_irq(13, routine::fpu_handler);
    associate_irq(14, routine::primary_ata_handler);
    associate_irq(15, routine::secondary_ata_handler);

    idtr_.limit
        = sizeof(entries_) - 1;
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

void idt::Manager::set_entry(int vector, void (*handler)(void*), LibC::uint8_t ist, LibC::uint8_t type_attr) noexcept
{
    auto addr = reinterpret_cast<LibC::uint64_t>(reinterpret_cast<void*>(handler));
    entries_[vector].offset_low = addr & 0xFFFF;
    entries_[vector].selector = 0x08;
    entries_[vector].ist = ist & 0x7;
    entries_[vector].type_attr = type_attr;
    entries_[vector].offset_middle = (addr >> 16) & 0xFFFF;
    entries_[vector].offset_high = (addr >> 32) & 0xFFFFFFFF;
    entries_[vector].zero = 0;
}

void Manager::register_exception(int vector, void (*handler)()) noexcept
{
    constexpr LibC::uint8_t TYPE_ATTR = 0x8E; // Interrupt Gate, Present, DPL=0
    LibC::uint8_t ist = isr_ist[vector];
    Logf(LogLevel::TRACE,
        "IDT Exception Registered: Vector=%d, Vector Name=%s, Handler=%p",
        vector, exception::vector_to_string(static_cast<exception::Vector>(vector)), handler);
    set_entry(vector, handler, ist, TYPE_ATTR);
}

void Manager::register_irq(int irq_number, void (*handler)()) noexcept
{
    constexpr LibC::uint8_t IST = 0;
    constexpr LibC::uint8_t TYPE_ATTR = 0x8E;

    int vector = 32 + irq_number; // Offset padrão IRQs no IDT

    set_entry(vector, handler, IST, TYPE_ATTR);
}

void Manager::associate_irq(int irq_number, void (*handler)(void*)) noexcept
{
    constexpr LibC::uint8_t IST = 0;
    constexpr LibC::uint8_t TYPE_ATTR = 0x8E; // Interrupt Gate, Present, DPL=0

    int vector = irq_number + 32;

    Logf(LogLevel::TRACE,
        "Associating IRQ %d to handler at %p (vector %d)",
        irq_number, handler, vector);

    set_entry(vector, handler, IST, TYPE_ATTR);
}

void Manager::register_syscall(int vector, void (*handler)()) noexcept
{
    constexpr LibC::uint8_t IST = 0;
    constexpr LibC::uint8_t TYPE_ATTR = 0xEF; // 1110 1111b (P=1, DPL=3, Trap Gate)

    set_entry(vector, handler, IST, TYPE_ATTR);
}

auto Manager::irq_get_handler(int irq_number) -> void (*)(void*)
{
    int vector = irq_number + 32;

    if (vector < 0 || vector >= IDT_ENTRIES)
        return nullptr;

    Entry& entry = entries_[vector];

    LibC::uint64_t addr = static_cast<LibC::uint64_t>(entry.offset_low)
        | (static_cast<LibC::uint64_t>(entry.offset_middle) << 16)
        | (static_cast<LibC::uint64_t>(entry.offset_high) << 32);

    if (addr == 0)
        return nullptr;

    return reinterpret_cast<void (*)(void*)>(addr);
}

}

extern "C" void irq_dispatch(LibC::uint8_t irq, void* context)
{
    auto handler = idt::Manager::instance().irq_get_handler(irq);
    if (handler) {
        handler(context);
    } else {
        Logf(LogLevel::WARN, "Unhandled IRQ %u", irq);
    }
}
