#include <Kernel/Arch/x86_64/gdt.h>
#include <LibFK/Log.hpp>

namespace gdt {

void Manager::set_entry(int index, LibC::uint32_t base, LibC::uint32_t limit, LibC::uint8_t access, LibC::uint8_t granularity) noexcept
{
    Entry& entry = entries_[index];

    entry.limit_low = limit & 0xFFFF;
    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFF;
    entry.access = access;
    entry.granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    entry.base_high = (base >> 24) & 0xFF;

    Logf(LogLevel::TRACE, "Gdt entry setted with %lu base, %d limit, %d access and %d granularity\n", base, limit, access, granularity);
}

void Manager::set_tss(LibC::uint64_t base, LibC::uint32_t limit) noexcept
{
    tss_entry_.limit_low = limit & 0xFFFF;
    tss_entry_.base_low = base & 0xFFFF;
    tss_entry_.base_middle = (base >> 16) & 0xFF;
    tss_entry_.access = 0x89; // present, DPL=0, type=9 (available 64-bit TSS)
    tss_entry_.granularity = (limit >> 16) & 0x0F;
    tss_entry_.base_high = (base >> 24) & 0xFF;
    tss_entry_.base_upper = (base >> 32) & 0xFFFFFFFF;
    tss_entry_.reserved = 0;

    Logf(LogLevel::TRACE, "TSS Entry setted with %d base and %d limit\n", base, limit);
}

void Manager::initialize() noexcept
{
    set_entry(0, 0, 0, 0, 0);

    set_entry(1, 0, 0xFFFFF, 0x9A, 0xA0);

    set_entry(2, 0, 0xFFFFF, 0x92, 0xC0);

    set_entry(3, 0, 0xFFFFF, 0xFA, 0xA0);

    set_entry(4, 0, 0xFFFFF, 0xF2, 0xC0);

    set_tss(reinterpret_cast<LibC::uint64_t>(&tss_), sizeof(TSS));

    gdtr_.limit = sizeof(entries_) + sizeof(tss_entry_) - 1;
    gdtr_.base = reinterpret_cast<LibC::uint64_t>(&entries_);

    *reinterpret_cast<TSSEntry*>(&entries_[5]) = tss_entry_;

    gdt_flush(&gdtr_);

    Log(LogLevel::INFO, "GDT Flushed with success");

    tss_flush(5 << 3); // Selector for TSS entry (index 5 << 3)

    Log(LogLevel::INFO, "TSS Flushed with success");
}

}
