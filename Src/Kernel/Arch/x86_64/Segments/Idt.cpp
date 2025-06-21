#include <Kernel/Arch/x86_64/exception.h>
#include <Kernel/Arch/x86_64/idt.h>
#include <Kernel/Arch/x86_64/isr.h>
#include <LibFK/Log.hpp>

namespace idt {

constexpr LibC::uint8_t IST_NONE = 0;
constexpr LibC::uint8_t IST_DOUBLE_FAULT = 1;

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
    constexpr LibC::uint8_t TYPE_ATTR = 0x8E; // Interrupt Gate, Present, DPL=0
    LibC::uint8_t ist = isr_ist[vector];      // Usa o IST correto conforme definido
    Logf(LogLevel::TRACE,
        "IDT Exception Registered: Vector=%d, Vector Name=%s, Handler=%p, IST=%u",
        vector, exception::vector_to_string(static_cast<exception::Vector>(vector)), handler, ist);
    set_entry(vector, handler, ist, TYPE_ATTR);
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
