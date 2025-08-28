#include <Kernel/Arch/x86_64/Cpu/Asm.h>
#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Cpu/Gdt_Constants.h>
#include <Kernel/Arch/x86_64/Segments/Gdt.h>
#include <LibFK/enforce.h>
#include <LibFK/log.h>

namespace gdt {

Manager& Manager::instance()
{
    static Manager instance;
    return instance;
}

void Manager::initialize() noexcept
{
    setup_tss();
    load_gdt();
    load_tss();
}

void Manager::set_descriptor(int index, LibC::uint32_t base, LibC::uint32_t limit,
    LibC::uint8_t access, LibC::uint8_t flags) noexcept
{
    constexpr int max_index = static_cast<int>(DescriptorCount);

    if (FK::alert_if_f(index < 0 || index >= max_index,
            "Gdt: set descriptor index %d out of bounds [0..%d]", index, max_index - 1)) {
        return;
    }

    Descriptor& desc = descriptors_[index];
    desc.limit_low = limit & 0xFFFF;
    desc.base_low = base & 0xFFFF;
    desc.base_middle = (base >> 16) & 0xFF;
    desc.access = access;
    desc.granularity = ((limit >> 16) & 0x0F) | (flags & 0xF0);
    desc.base_high = (base >> 24) & 0xFF;
}

void Manager::set_tss_descriptor(int index, LibC::uint64_t base, LibC::uint32_t limit) noexcept
{
    constexpr int max_index = static_cast<int>(DescriptorCount) - 1;

    FK::enforcef(index >= 0 && index < max_index,
        "Gdt: set tss descriptor: index %d out of bounds or no space for TSS", index);

    Descriptor& low = descriptors_[index];
    Descriptor& high = descriptors_[index + 1];

    low.limit_low = limit & 0xFFFF;
    low.base_low = static_cast<LibC::uint32_t>(base & 0xFFFF);
    low.base_middle = static_cast<LibC::uint8_t>((base >> 16) & 0xFF);
    low.access = 0x89; // Presente, tipo 9 (TSS disponível)
    low.granularity = (limit >> 16) & 0x0F;
    low.base_high = static_cast<LibC::uint8_t>((base >> 24) & 0xFF);

    high.limit_low = static_cast<LibC::uint16_t>((base >> 32) & 0xFFFF);
    high.base_low = static_cast<LibC::uint16_t>((base >> 48) & 0xFFFF);
    high.base_middle = 0;
    high.access = 0;
    high.granularity = 0;
    high.base_high = 0;
}

void Manager::setup_tss() noexcept
{
    LibC::memset(&tss_, 0, sizeof(tss_));

    tss_.rsp0 = reinterpret_cast<LibC::uint64_t>(kernel_stack) + sizeof(kernel_stack);
    tss_.ist[0] = reinterpret_cast<LibC::uint64_t>(ist_stack_nmi) + sizeof(ist_stack_nmi);
    tss_.ist[1] = reinterpret_cast<LibC::uint64_t>(ist_stack_double_fault) + sizeof(ist_stack_double_fault);
    tss_.ist[2] = reinterpret_cast<LibC::uint64_t>(ist_stack_machine_check) + sizeof(ist_stack_machine_check);

    tss_.io_map_base = sizeof(Tss::Tss);

    LibC::uint64_t base = reinterpret_cast<LibC::uint64_t>(&tss_);
    LibC::uint32_t limit = sizeof(Tss::Tss) - 1;

    set_tss_descriptor(static_cast<int>(DescriptorIndex::FirstAvailableTSS), base, limit);
}

void Manager::load_gdt() noexcept
{
    gdtr_.limit = static_cast<LibC::uint16_t>(sizeof(descriptors_) - 1);
    gdtr_.base = reinterpret_cast<LibC::uint64_t>(&descriptors_);

    set_descriptor(static_cast<int>(DescriptorIndex::Null), 0, 0, 0, 0);
    set_descriptor(static_cast<int>(DescriptorIndex::KernelCode), 0, 0xFFFFF, SEGMENT_CODE_RING0, GRANULARITY_FLAGS);
    set_descriptor(static_cast<int>(DescriptorIndex::KernelData), 0, 0xFFFFF, SEGMENT_DATA_RING0, GRANULARITY_FLAGS);
    set_descriptor(static_cast<int>(DescriptorIndex::UserCode), 0, 0xFFFFF, SEGMENT_CODE_RING3, GRANULARITY_FLAGS);
    set_descriptor(static_cast<int>(DescriptorIndex::UserData), 0, 0xFFFFF, SEGMENT_DATA_RING3, GRANULARITY_FLAGS);

    flush_gdt(&gdtr_);

    Logf(LogLevel::INFO, "GDT: Loaded GDTR (limit=0x%04X, base=0x%016llX)",
        gdtr_.limit, static_cast<unsigned long long>(gdtr_.base));
}

void Manager::load_tss() noexcept
{
    flush_tss(TSS_SELECTOR);
    Logf(LogLevel::INFO, "TSS: Loaded selector %p", TSS_SELECTOR);
}

}
