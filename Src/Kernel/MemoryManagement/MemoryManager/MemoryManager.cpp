#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/MemoryManager/MemoryManager.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibFK/types.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif // DEBUG

namespace MemoryManagement {

void MemoryManager::initialize(multiboot2::TagMemoryMap const& mmap)
{
    FK::enforcef(!is_initialized, "MemoryManager: initialize called multiple times");
    auto& pmm = PhysicalMemoryManager::instance();
    auto& vmm = VirtualMemoryManager::instance();

    for (auto it = mmap.begin(); it != mmap.end(); ++it) {
        auto const& entry = *it;

        if (entry.length < TOTAL_MEMORY_PAGE_SIZE || entry.base_addr < static_cast<FK::qword>(1 * FK::MiB)) {
            continue;
        }

        LibC::uintptr_t base = (entry.base_addr + TOTAL_MEMORY_PAGE_SIZE - 1) & ~(TOTAL_MEMORY_PAGE_SIZE - 1);
        LibC::uintptr_t end = entry.base_addr + entry.length;
        if (end <= base) {
            continue; // Nada para mapear após alinhamento
        }

        LibC::uint64_t total_pages = (end - base) / TOTAL_MEMORY_PAGE_SIZE;
        if (total_pages == 0) {
            continue;
        }

        while (total_pages > 0) {
            LibC::uint64_t region_pages = (total_pages > max_region_in_pages) ? max_region_in_pages : total_pages;

            PhysicalMemoryRegion* region = PhysicalMemoryManager::allocate_region(base, region_pages);
            FK::enforcef(region != nullptr, "MemoryManager: Failed to allocate PhysicalMemoryRegion for base=%p, pages=%lu", base, region_pages);

            pmm.add_region(region);

            if (!multiboot2::is_available(entry.type)) {
                if (region->allocated && !region->bitmap.is_valid()) {
                    pmm.ensure_bitmap_allocated(*region);
                    pmm.mark_pages(*region, 0, region_pages, true);
                }
            }

            base += region_pages * TOTAL_MEMORY_PAGE_SIZE;
            total_pages -= region_pages;
        }
    }

    // TODO: add some form of check if mapping is occur properly
    for (auto& region : pmm.regions()) {
        if (!region.is_allocated())
            continue;

        LibC::uintptr_t base_phys = region.base_addr;
        LibC::uint64_t page_count = region.page_count;
        bool is_available = false;

        for (LibC::uint64_t i = 0; i < page_count; ++i) {
            LibC::uintptr_t phys_addr = base_phys + i * TOTAL_MEMORY_PAGE_SIZE;
            if (phys_addr == 0)
                continue;

            auto virt_addr_signed = static_cast<LibC::intptr_t>(phys_addr) + PAGE_MASK;
            LibC::uintptr_t virt_addr = static_cast<LibC::uintptr_t>(virt_addr_signed);

            if ((virt_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) != 0) {
                Logf(LogLevel::WARN, "MemoryManager: virt_addr unaligned: 0x%lx", virt_addr);
                continue;
            }

            for (auto it = mmap.begin(); it != mmap.end(); ++it) {
                auto const& entry = *it;
                LibC::uintptr_t entry_start = entry.base_addr;
                LibC::uintptr_t entry_end = entry.base_addr + entry.length;

                if (phys_addr >= entry_start && phys_addr < entry_end) {
                    is_available = multiboot2::is_available(entry.type);
                    break;
                }
            }

            LibC::uint64_t flags = PAGE_PRESENT;
            if (is_available)
                flags |= PAGE_RW;

            bool mapped = vmm.map_page(virt_addr, phys_addr, flags);
            FK::alert_if_f(!mapped, "MemoryManager: Failed to map page: virt=0x%lx phys=0x%lx", virt_addr, phys_addr);
        }
    }

    is_initialized = true;
    Log(LogLevel::INFO, "MemoryManager: initialized with successfully");
}
}
