#pragma once

#include <Kernel/Arch/x86_64/Cpu/Asm.h>
#include <Kernel/Arch/x86_64/Cpu/Idt_Constants.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace idt {

using IrqHandler = void (*)(LibC::uint8_t irq, void* context);

struct idt_entry {
    LibC::uint16_t offset_low;
    LibC::uint16_t selector;
    LibC::uint8_t ist;
    LibC::uint8_t type_attr;
    LibC::uint16_t offset_mid;
    LibC::uint32_t offset_high;
    LibC::uint32_t zero;
} __attribute__((packed));

static_assert(sizeof(idt_entry) == 16, "idt_entry must be 16 bytes");

struct idt_pointer {
    LibC::uint16_t limit;
    LibC::uint64_t base;
} __attribute__((packed, aligned(1)));

static_assert(sizeof(idt_pointer) == 10, "idt_pointer must be 10 bytes");
class Manager {
private:
    idt_entry entries_[MAX_IDT_ENTRIES];
    idt_pointer idtr;

    Manager() = default;
    Manager(Manager const&) = delete;
    Manager& operator=(Manager const&) = delete;

private:
    void validate_index(int index) const noexcept;

    void validate_irq(int irq) const noexcept;

    void enable_irqs() noexcept;

public:
    static inline Manager& Instance()
    {
        static Manager instance;
        return instance;
    }

    void initialize() noexcept;
    void set_entry(int index, void* isr, LibC::uint16_t selector, LibC::uint8_t type_attr, LibC::uint8_t ist) noexcept;

    void send_eoi(int irq) noexcept;
    void register_exception(int vector) noexcept;
    void register_irq(int irq) noexcept;
};

extern "C" void irq_dispatch(LibC::uint8_t irq, void* context) noexcept;

}
