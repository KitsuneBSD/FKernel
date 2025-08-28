#include <Kernel/Arch/x86_64/Interrupts/Exceptions.h>
#include <Kernel/Arch/x86_64/Interrupts/Interrupt_Constants.h>
#include <Kernel/Arch/x86_64/Interrupts/Isr.h>
#include <LibFK/enforce.h>

void (*exception_stubs[MAX_EXCEPTION_ID])() = {
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

LibC::uint8_t const isr_ist[MAX_EXCEPTION_ID] = {
    IST_NONE, IST_NONE, IST_NMI, IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_DOUBLE_FAULT, IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE,
    IST_NONE, IST_NONE, IST_NONE, IST_NONE, IST_NONE
};

constexpr char const* const names[] = {
    "Divide By Zero",
    "Debug",
    "Non Maskable Interrupt",
    "Breakpoint",
    "Overflow",
    "Bound Range Exceeded",
    "Invalid Opcode",
    "Device Not Available",
    "Double Fault",
    "Coprocessor Segment Overrun",
    "Invalid TSS",
    "Segment Not Present",
    "Stack-Segment Fault",
    "General Protection Fault",
    "Page Fault",
    "Reserved",
    "x87 Floating-Point Exception",
    "Alignment Check",
    "Machine Check",
    "SIMD Floating-Point Exception",
    "Virtualization Exception",
    "Control Protection Exception",
    "Reserved 22",
    "Reserved 23",
    "Reserved 24",
    "Reserved 25",
    "Reserved 26",
    "Reserved 27",
    "Reserved 28",
    "Reserved 29",
    "Reserved 30",
    "Reserved 31"
};

char const* named_exception(int index) noexcept
{
    constexpr int count = static_cast<int>(sizeof(names) / sizeof(names[0]));
    FK::enforcef(index >= 0 && index < count, "named_exception: invalid exception index %d", index);
    return names[index];
}
