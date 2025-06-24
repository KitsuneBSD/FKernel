#pragma once

extern "C" void flush_gdt(void* gdtr);

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace gdt {
enum GdtIndex : int {
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
    gdt_entry entries_[5];
    gdt_pointer gdtr;

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
};

}
