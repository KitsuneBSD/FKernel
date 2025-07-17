#include "Kernel/Arch/x86_64/Cpu/Constants.h"
#include "LibFK/enforce.h"
#include <Kernel/Arch/x86_64/Cpu/Asm.h>
#include <Kernel/Arch/x86_64/Segments/Gdt.h>
#include <LibFK/log.h>

namespace gdt {

alignas(16) static LibC::uint8_t ist1_stack[4096]; // NMI
alignas(16) static LibC::uint8_t ist2_stack[4096]; // Double Fault
alignas(16) static LibC::uint8_t ist3_stack[4096]; // Extra (ex: Machine Check)
alignas(16) static LibC::uint64_t kernel_stack[16384];

void Manager::set_entry(int index, LibC::uint32_t base, LibC::uint32_t limit,
    LibC::uint8_t access, LibC::uint8_t granularity) noexcept
{
    constexpr int max_entries = sizeof(entries_) / sizeof(entries_[0]);
    FK::enforcef(index >= 0 && index < max_entries,
        "GDT::set_entry: index %d out of bounds [0..%d]", index, max_entries - 1);

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
    constexpr int max_entries = sizeof(entries_) / sizeof(entries_[0]);
    FK::enforcef(index >= 0 && (index + 1) < max_entries,
        "GDT::set_tss_entry: index %d out of bounds or no space for TSS descriptor", index);

    auto& low = entries_[index];
    auto& next = entries_[index + 1];

    low.limit_low = limit & 0xFFFF;
    low.base_low = base & 0xFFFF;
    low.base_middle = (base >> 16) & 0xFF;
    low.access = 0x89;                      // P=1, DPL=0, S=0, Type=1001 (TSS available 64-bit)
    low.granularity = (limit >> 16) & 0x0F; // Byte granularity, no G flag
    low.base_high = (base >> 24) & 0xFF;

    next.limit_low = (base >> 32) & 0xFFFF;
    next.base_low = (base >> 48) & 0xFFFF;
    next.base_middle = 0;
    next.access = 0;
    next.granularity = 0;
    next.base_high = 0;

    Logf(LogLevel::TRACE, "GDT: TSS set at index %d (base=0x%016llX, limit=0x%08X)",
        index, static_cast<unsigned long long>(base), limit);
}

void Manager::initialize() noexcept
{
    Logf(LogLevel::INFO, "GDT: Initializing Global Descriptor Table for x86_64 (64-bit)");

    set_entry(GdtIndex::Null, 0, 0xFFFFF, 0, 0);
    set_entry(GdtIndex::KernelCode, 0, 0xFFFFF, 0x9A, 0xA0); // Executable, ring 0
    set_entry(GdtIndex::KernelData, 0, 0xFFFFF, 0x92, 0xA0); // Data, ring 0
    set_entry(GdtIndex::UserCode, 0, 0xFFFFF, 0xFA, 0xA0);   // Executable, ring 3
    set_entry(GdtIndex::UserData, 0, 0xFFFFF, 0xF2, 0xA0);   // Data, ring 3

    Logf(LogLevel::INFO, "TSS: Initializing Task State Segment for x86_64 (64-bit)");

    // Correct pointer arithmetic: add byte size to pointer cast to integer
    tss_.rsp0 = reinterpret_cast<LibC::uint64_t>(kernel_stack) + sizeof(kernel_stack);
    tss_.ist[0] = reinterpret_cast<LibC::uint64_t>(ist1_stack) + sizeof(ist1_stack);
    tss_.ist[1] = reinterpret_cast<LibC::uint64_t>(ist2_stack) + sizeof(ist2_stack);
    tss_.ist[2] = reinterpret_cast<LibC::uint64_t>(ist3_stack) + sizeof(ist3_stack);

    tss_.io_map_base = sizeof(Tss::Tss);

    LibC::uint64_t const tss_base = reinterpret_cast<LibC::uint64_t>(&tss_);
    LibC::uint32_t const tss_limit = sizeof(Tss::Tss) - 1;

    set_tss_entry(5, tss_base, tss_limit);

    gdtr.limit = sizeof(entries_) - 1;
    gdtr.base = reinterpret_cast<LibC::uint64_t>(&entries_);

    flush_gdt(&gdtr);
    Logf(LogLevel::INFO, "GDT: Loaded successfully");

    flush_tss(TSS_SELECTOR);
    Logf(LogLevel::INFO, "TSS: Loaded successfully");
}

}
