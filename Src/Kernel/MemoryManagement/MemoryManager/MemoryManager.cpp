#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/MemoryManager/MemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stdint.h>
#include <LibFK/enforce.h>
#include <LibFK/intrusiveList.h>
#include <LibFK/log.h>
#include <LibFK/new.h>
#include <LibFK/pair.h>
#include <LibFK/types.h>
#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Asm.h>
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif // DEBUG

namespace MemoryManagement {

extern "C" char __kernel_end[];

// TODO: Put this code in a LibC::ceil function
static constexpr LibC::uintptr_t align_down(LibC::uintptr_t value, LibC::uintptr_t alignment) noexcept
{
    return value & ~(alignment - 1);
}

MMapCacheEntry* MemoryManager::find_cache_entry(LibC::uintptr_t phys_addr) noexcept
{
    for (auto* node = mmap_cache.head(); node != nullptr; node = node->list_node.next) {
        auto& [base, entry] = node->data;
        LibC::uintptr_t aligned_base = base;
        LibC::uintptr_t aligned_end = align_down(base + entry.length, TOTAL_MEMORY_PAGE_SIZE);

        if (phys_addr >= aligned_base && phys_addr < aligned_end) {
            return node;
        }
    }
    return nullptr;
}

void MemoryManager::initialize(multiboot2::TagMemoryMap const& mmap)
{
    auto& pmm = PhysicalMemoryManager::instance();

    for (auto it = mmap.begin(); it != mmap.end(); ++it) {
        auto const& entry = *it;

        if (entry.length < TOTAL_MEMORY_PAGE_SIZE || entry.base_addr < static_cast<FK::qword>(1 * FK::MiB)) {
            continue;
        }

        LibC::uintptr_t base = entry.base_addr;
        LibC::uint64_t total_pages = entry.length / TOTAL_MEMORY_PAGE_SIZE;

        while (total_pages > 0) {
            LibC::uint64_t region_pages = (total_pages > max_region_in_pages) ? max_region_in_pages : total_pages;

            PhysicalMemoryRegion* region = pmm.allocate_region(base, region_pages);
            if (FK::alert_if_f(region == nullptr, "PMM: Failed to allocate PhysicalMemoryRegion for base=%p, pages=%lu", base, region_pages))
                continue;

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

    Log(LogLevel::INFO, "PMM: Initialized with success");
    pmm.log_memory_status();

    this->is_initialized = true;
}

} // namespace MemoryManagement
