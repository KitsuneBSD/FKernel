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

void Manager::initialize() noexcept
{

    Logf(LogLevel::INFO, "GDT: Initialize Global Descriptor Table from x86_64 (64 Bits)");

    set_entry(GdtIndex::Null, 0, 0, 0, 0);
    set_entry(GdtIndex::KernelCode, 0, 0, 0x9A, 0xA0); // Executável, ring 0
    set_entry(GdtIndex::KernelData, 0, 0, 0x92, 0xA0); // Dados, ring 0
    set_entry(GdtIndex::UserCode, 0, 0, 0xFA, 0xA0);   // Executável, ring 3
    set_entry(GdtIndex::UserData, 0, 0, 0xF2, 0xA0);   // Dados, ring 3

    gdtr.limit = sizeof(entries_) - 1;
    gdtr.base = reinterpret_cast<LibC::uint64_t>(&entries_);

    flush_gdt(&gdtr);

    Log(LogLevel::INFO, "GDT: Loaded with success");
}

}
