#pragma once

#include <Kernel/Arch/x86_64/stack_size.h>
#include <LibC/stdint.h>

namespace gdt {

struct [[gnu::packed]] Entry {
    LibC::uint16_t limit_low;
    LibC::uint16_t base_low;
    LibC::uint8_t base_middle;
    LibC::uint8_t access;
    LibC::uint8_t granularity;
    LibC::uint8_t base_high;
};

static_assert(sizeof(Entry) == 8, "GDT::Entry must be 8 bytes");

struct [[gnu::packed]] TSSEntry {
    LibC::uint16_t limit_low;
    LibC::uint16_t base_low;
    LibC::uint8_t base_middle;
    LibC::uint8_t access;
    LibC::uint8_t granularity;
    LibC::uint8_t base_high;
    LibC::uint32_t base_upper;
    LibC::uint32_t reserved;
};

static_assert(sizeof(TSSEntry) == 16, "GDT::TSSEntry must be 16 bytes");

struct [[gnu::packed]] Pointer {
    LibC::uint16_t limit;
    LibC::uint64_t base;
};

static_assert(sizeof(Pointer) == 10, "GDT::Pointer must be 10 bytes");

struct [[gnu::packed]] TSS {
    LibC::uint32_t reserved0 = 0;
    LibC::uint64_t rsp0 = 0;
    LibC::uint64_t rsp1 = 0;
    LibC::uint64_t rsp2 = 0;
    LibC::uint64_t reserved1 = 0;
    LibC::uint64_t ist[IST_COUNT] = {};
    LibC::uint64_t reserved2 = 0;
    LibC::uint16_t reserved3 = 0;
    LibC::uint16_t io_map_base = 0;
};

static_assert(sizeof(TSS) == 104, "GDT::TSS must be 104 bytes (64-bit TSS)");

class Manager final {
public:
    static Manager& instance() noexcept
    {
        static Manager gdt_instance;
        return gdt_instance;
    }

    void initialize() noexcept;
    void set_entry(int index, LibC::uint32_t base, LibC::uint32_t limit, LibC::uint8_t access, LibC::uint8_t granularity) noexcept;
    void set_tss(LibC::uint64_t base, LibC::uint32_t limit) noexcept;

    Pointer const& gdtr() const noexcept { return gdtr_; }
    const TSS& tss() const noexcept { return tss_; }

private:
    Manager() noexcept = default;

    Entry entries_[5] = {};
    TSSEntry tss_entry_ = {};
    Pointer gdtr_ = {};
    TSS tss_ = {};
};

extern "C" void gdt_flush(Pointer const* gdtr);
extern "C" void tss_flush(LibC::uint16_t selector);
}
