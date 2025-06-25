#include "Kernel/Arch/x86_64/Cpu/Constants.h"
#include <Kernel/Arch/x86_64/Cpu/Asm.h>
#include <Kernel/Arch/x86_64/Segments/Gdt.h>
#include <LibFK/Log.h>

namespace gdt {

void Manager::set_entry(int index, LibC::uint32_t base, LibC::uint32_t limit, LibC::uint8_t access, LibC::uint8_t granularity) noexcept
{
    auto& entry = entries_[index];

    entry.limit_low = limit & 0xFFFF;
    entry.base_low = base & 0xFFFF;
    entry.base_middle = (base >> 16) & 0xFF;
    entry.access = access;
    entry.granularity = ((limit >> 16) & 0x0F) | (granularity & 0xF0);
    entry.base_high = (base >> 24) & 0xFF;

    Logf(LogLevel::TRACE,
        "GDT[%d] = base: 0x%08X, limit: 0x%08X, access: 0x%02X, gran: 0x%02X",
        index, base, limit, access, granularity);
}
void Manager::set_tss_entry(int index, LibC::uint64_t base, LibC::uint32_t limit) noexcept
{
    auto& low = entries_[index];
    auto& next = entries_[index + 1];

    low.limit_low = limit & 0xFFFF;
    low.base_low = base & 0xFFFF;
    low.base_middle = (base >> 16) & 0xFF;
    low.access = 0x89; // P=1, DPL=0, S=0, Type=1001 (TSS available 64-bit)
    low.granularity = (limit >> 16) & 0x0F;
    low.granularity |= 0; // Sem granularity (byte granularity)
    low.base_high = (base >> 24) & 0xFF;

    next.limit_low = (base >> 32) & 0xFFFF;
    next.base_low = (base >> 48) & 0xFFFF;
    next.base_middle = 0;
    next.access = 0;
    next.granularity = 0;
    next.base_high = 0;

    Logf(LogLevel::TRACE, "GDT: TSS Set in index %d (base=0x%016llx, limit=0x%08x)",
        index,
        static_cast<unsigned long long>(base),
        limit);
}

void Manager::initialize() noexcept
{

    Logf(LogLevel::INFO, "GDT: Initialize Global Descriptor Table from x86_64 (64 Bits)");

    set_entry(GdtIndex::Null, 0, 0xFFFFF, 0, 0);
    set_entry(GdtIndex::KernelCode, 0, 0xFFFFF, 0x9A, 0xA0); // Executável, ring 0
    set_entry(GdtIndex::KernelData, 0, 0xFFFFF, 0x92, 0xA0); // Dados, ring 0
    set_entry(GdtIndex::UserCode, 0, 0xFFFFF, 0xFA, 0xA0);   // Executável, ring 3
    set_entry(GdtIndex::UserData, 0, 0xFFFFF, 0xF2, 0xA0);   // Dados, ring 3

    Logf(LogLevel::INFO, "TSS: Initialize Task State Segment from x86_64 (64 Bits)");
    LibC::uint64_t const tss_base = reinterpret_cast<LibC::uint64_t>(&tss_);
    LibC::uint32_t const tss_limit = sizeof(Tss::Tss) - 1;

    set_tss_entry(5, tss_base, tss_limit);

    gdtr.limit = sizeof(entries_) - 1;
    gdtr.base = reinterpret_cast<LibC::uint64_t>(&entries_);

    flush_gdt(&gdtr);

    Log(LogLevel::INFO, "GDT: Loaded with success");

    flush_tss(TSS_SELECTOR);

    Log(LogLevel::INFO, "TSS: Loaded with success");
}

}
