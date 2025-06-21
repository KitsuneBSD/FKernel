#pragma once

#include <Kernel/Arch/x86_64/stack_size.h>
#include <LibC/stdint.h>

namespace idt {
struct [[gnu::packed]] Entry {
    LibC::uint16_t offset_low;    // bits 0-15 do handler
    LibC::uint16_t selector;      // seletor do segmento (GDT)
    LibC::uint8_t ist;            // bits IST (3 bits), outros zero
    LibC::uint8_t type_attr;      // tipo e atributos
    LibC::uint16_t offset_middle; // bits 16-31 do handler
    LibC::uint32_t offset_high;   // bits 32-63 do handler
    LibC::uint32_t zero;          // reservado, zero
};

static_assert(sizeof(Entry) == 16, "IDT::Entry must be 16 bytes");

struct [[gnu::packed]] Pointer {
    LibC::uint16_t limit;
    LibC::uint64_t base;
};

static_assert(sizeof(Pointer) == 10, "IDT::Pointer must be 10 bytes");

class Manager final {
private:
    Manager() = default;
    Entry entries_[IDT_ENTRIES];
    Pointer idtr_ = {};

public:
    static Manager& instance() noexcept
    {
        static Manager instance;
        return instance;
    }

    void initialize() noexcept;
    void set_entry(int vector, void (*handler)(), LibC::uint8_t ist, LibC::uint8_t type_attr) noexcept;
    void set_entry(int vector, void (*handler)(void*), LibC::uint8_t ist, LibC::uint8_t type_attr) noexcept;

    void register_exception(int vector, void (*handler)()) noexcept;
    void register_irq(int irq_number, void (*handler)()) noexcept;
    void register_syscall(int vector, void (*handler)()) noexcept;

    void associate_irq(int irq_number, void (*handler)(void*)) noexcept;
    Pointer const& idtr() const noexcept { return idtr_; }

    auto irq_get_handler(int irq_number) -> void (*)(void*);
};

extern "C" void idt_flush(Pointer const* idtr);
}

extern "C" void irq_dispatch(LibC::uint8_t irq, void* context);
