#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/MemoryManager/MemoryManager.h>
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

MMapCacheEntry* MemoryManager::find_cache_entry(LibC::uintptr_t phys_addr) noexcept
{
    for (auto* node = mmap_cache.head(); node != nullptr; node = node->list_node.next) {
        auto& [base, entry] = node->data;
        if (phys_addr >= base && phys_addr < base + entry.length) {
            return node;
        }
    }
    return nullptr;
}

void MemoryManager::populate_cache(multiboot2::TagMemoryMap const& mmap) noexcept
{
    Log(LogLevel::TRACE, "MemoryManager: Starting to populate mmap_cache");

    for (auto it = mmap.begin(); it != mmap.end(); ++it) {
        auto const& entry = *it;

        LibC::uintptr_t base = (entry.base_addr + TOTAL_MEMORY_PAGE_SIZE - 1) & ~(TOTAL_MEMORY_PAGE_SIZE - 1);

        void* mem = Falloc(sizeof(MMapCacheEntry), alignof(MMapCacheEntry));
        FK::enforcef(mem != nullptr, "MemoryManager: Failed to allocate MMapCacheEntry");

        auto* cache_entry = new (mem) MMapCacheEntry {
            FK::Pair(base, entry),
            FK::IntrusiveNode<MMapCacheEntry> {}
        };

        mmap_cache.append(cache_entry);

        Logf(LogLevel::TRACE,
            "MemoryManager: Cached mmap entry base=0x%lx, aligned_base=0x%lx, length=0x%lx, type=%d",
            entry.base_addr, base, entry.length, static_cast<int>(entry.type));
    }

    Log(LogLevel::TRACE, "MemoryManager: Finished populating mmap_cache");
}

void MemoryManager::allocate_and_map() noexcept
{
    auto& pmm = PhysicalMemoryManager::instance();
    auto& vmm = VirtualMemoryManager::instance();

    LibC::uintptr_t kernel_end_phys = reinterpret_cast<LibC::uintptr_t>(&__kernel_end);

    for (auto& cache_entry : mmap_cache) {
        auto const& data = cache_entry.data.second;

        if (FK::alert_if_f(data.length < TOTAL_MEMORY_PAGE_SIZE,
                "MemoryManager: skipping region with length less than page size"))
            continue;

        if (FK::alert_if_f(data.base_addr < static_cast<FK::qword>(1 * FK::MiB),
                "MemoryManager: skipping region below 1 MiB boundary"))
            continue;

        LibC::uintptr_t base = (data.base_addr + TOTAL_MEMORY_PAGE_SIZE - 1) & ~(TOTAL_MEMORY_PAGE_SIZE - 1);
        LibC::uintptr_t end = data.base_addr + data.length;

        if (FK::alert_if_f(end <= base,
                "MemoryManager: skipping region with end <= base after alignment"))
            continue;

        LibC::uint64_t total_pages = (end - base) / TOTAL_MEMORY_PAGE_SIZE;

        if (FK::alert_if_f(total_pages == 0,
                "MemoryManager: skipping region with zero pages"))
            continue;

        while (total_pages > 0) {
            LibC::uint64_t region_pages = (total_pages > max_region_in_pages) ? max_region_in_pages : total_pages;

            PhysicalMemoryRegion* region = PhysicalMemoryManager::allocate_region(base, region_pages);

            if (FK::alert_if_f(region == nullptr,
                    "MemoryManager: failed to allocate PhysicalMemoryRegion for base=%p, pages=%lu", base, region_pages))
                break;

            pmm.add_region(region);

            if (!multiboot2::is_available(data.type)) {
                if (region->allocated && !region->bitmap.is_valid()) {
                    pmm.ensure_bitmap_allocated(*region);
                    pmm.mark_pages(*region, 0, region_pages, true);
                }
            }

            base += region_pages * TOTAL_MEMORY_PAGE_SIZE;
            total_pages -= region_pages;
        }
    }

    for (auto& region : pmm.regions()) {
        if (FK::alert_if_f(!region.is_allocated(),
                "MemoryManager: skipping unallocated region base=%p", region.base_addr))
            continue;

        LibC::uintptr_t base_phys = region.base_addr;
        LibC::uint64_t page_count = region.page_count;

        for (LibC::uint64_t i = 0; i < page_count; ++i) {
            LibC::uintptr_t phys_addr = base_phys + i * TOTAL_MEMORY_PAGE_SIZE;

            if (FK::alert_if_f(phys_addr == 0,
                    "MemoryManager: skipping page with physical address 0"))
                continue;

            auto virt_addr_signed = static_cast<LibC::intptr_t>(phys_addr) + PAGE_MASK;
            LibC::uintptr_t virt_addr = static_cast<LibC::uintptr_t>(virt_addr_signed);

            if (FK::alert_if_f((virt_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) != 0,
                    "MemoryManager: virt_addr unaligned: 0x%lx", virt_addr))
                continue;

            MMapCacheEntry* cache_entry = find_cache_entry(phys_addr);
            bool is_available = cache_entry ? multiboot2::is_available(cache_entry->data.second.type) : false;

            LibC::uint64_t flags = PAGE_PRESENT;
            LibC::uint64_t usable_flags = flags;

            if (is_available && phys_addr <= kernel_end_phys) {
                usable_flags |= PAGE_RW;
            } else {
                usable_flags |= PAGE_RW | PAGE_USER;
            }

            if (FK::alert_if_f(!vmm.map_page(virt_addr, phys_addr, usable_flags),
                    "MemoryManager: failed to map page: virt=%p phys=%p", virt_addr, phys_addr))
                continue;
        }
    }
}

void MemoryManager::initialize(multiboot2::TagMemoryMap const& mmap)
{
    auto& vmm = VirtualMemoryManager::instance();
    FK::enforcef(!is_initialized, "MemoryManager: Initialize called multiple times");

    if (vmm.pml4 == nullptr) {
        vmm.pml4 = reinterpret_cast<LibC::uint64_t*>(current_pml4_ptr);
    }

    populate_cache(mmap);

    allocate_and_map();

    current_pml4_ptr = reinterpret_cast<LibC::uint64_t*>(vmm.pml4);
    is_initialized = true;

    Log(LogLevel::INFO, "MemoryManager: Initialized with successfully");
}

}
