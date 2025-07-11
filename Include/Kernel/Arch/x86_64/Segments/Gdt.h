#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#include <Kernel/Arch/x86_64/Cpu/Asm.h>
#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Segments/Tss.h>

namespace gdt {
enum GdtIndex : LibC::uint8_t {
    Null = 0,
    KernelCode,
    KernelData,
    UserCode,
    UserData,
    EntryCount
};

struct gdt_entry {
    LibC::uint16_t limit_low;
    LibC::uint16_t base_low;
    LibC::uint8_t base_middle;
    LibC::uint8_t access;
    LibC::uint8_t granularity;
    LibC::uint8_t base_high;
} __attribute__((packed));

static_assert(sizeof(gdt::gdt_entry) == 8, "gdt_entry must be 8 bytes");

struct gdt_pointer {
    LibC::uint16_t limit;
    LibC::uint64_t base;
} __attribute__((packed, aligned(1)));

static_assert(sizeof(gdt::gdt_pointer) == 10, "gdt_pointer must be 10 bytes");

// Manager is a singleton class (in this moment) to set the global descriptor table
class Manager {
private:
    gdt_entry entries_[IST_COUNT];
    gdt_pointer gdtr;
    Tss::Tss tss_;

    Manager() = default;

    Manager(Manager const&) = delete;
    Manager& operator=(Manager const&) = delete;

public:
    static inline Manager& Instance()
    {
        static Manager instance;
        return instance;
    }

    void initialize() noexcept;
    void set_entry(int index, LibC::uint32_t base, LibC::uint32_t limit, LibC::uint8_t access, LibC::uint8_t granularity) noexcept;
    void set_tss_entry(int index, LibC::uint64_t base, LibC::uint32_t limit) noexcept;
};

}
