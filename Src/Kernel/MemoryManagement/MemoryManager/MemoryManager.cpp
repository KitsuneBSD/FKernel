#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManagement/MemoryManager/MemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
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

constexpr LibC::uintptr_t KERNEL_VIRT_BASE = 0xffff800000000000;                // base virtual onde a RAM é mapeada
constexpr LibC::uintptr_t KERNEL_DIRECT_MAP_SIZE = 512ull * 1024 * 1024 * 1024; // 512 GiB

// TODO: Put this code in a LibC::floor function
static constexpr LibC::uintptr_t align_down(LibC::uintptr_t value, LibC::uintptr_t alignment) noexcept
{
    return value & ~(alignment - 1);
}

// TODO: Put this code in a LibC::ceil function
static constexpr LibC::uintptr_t align_up(LibC::uintptr_t value, LibC::uintptr_t alignment) noexcept
{
    return (value + alignment - 1) & ~(alignment - 1);
}

constexpr LibC::uintptr_t phys_to_virt(LibC::uintptr_t phys)
{
    return (phys < KERNEL_DIRECT_MAP_SIZE)
        ? reinterpret_cast<LibC::uintptr_t>(phys + KERNEL_VIRT_BASE)
        : 0;
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
    auto& vmm = VirtualMemoryManager::instance();

    vmm.pml4 = reinterpret_cast<LibC::uint64_t*>(read_cr3());

    for (auto it = mmap.begin(); it != mmap.end(); ++it) {
        auto const& entry = *it;

        if (entry.length < TOTAL_MEMORY_PAGE_SIZE || entry.base_addr < static_cast<FK::qword>(1 * FK::MiB)) {
            continue;
        }

        LibC::uintptr_t base = align_up(entry.base_addr, TOTAL_MEMORY_PAGE_SIZE);
        LibC::uint64_t total_pages = entry.length / TOTAL_MEMORY_PAGE_SIZE;

        while (total_pages > 0) {
            LibC::uint64_t region_pages = (total_pages > max_region_in_pages) ? max_region_in_pages : total_pages;

            PhysicalMemoryRegion* region = pmm.allocate_region(base, region_pages);
            if (FK::alert_if_f(region == nullptr, "PMM: Failed to allocate PhysicalMemoryRegion for base=%p, pages=%lu", base, region_pages))
                continue;

            pmm.add_region(region);
            auto virt = phys_to_virt(region->base_addr);

            if (FK::alert_if_f(virt == 0, "VMM: phys_to_virt unsafe for base=%p", region->base_addr)) {
                pmm.remove_region(region);
                continue;
            }

            LibC::uint64_t flags = PAGE_PRESENT;

            if (multiboot2::is_available(entry.type)) {
                flags |= PAGE_RW;
            } else {
                pmm.ensure_bitmap_allocated(*region);
                pmm.mark_pages(*region, 0, region_pages, true);
            }

            if (FK::alert_if_f(!vmm.map_page(virt, region->base_addr, flags), "VMM: map_page failed for base=%p", region->base_addr)) {
                pmm.remove_region(region);
                continue;
            }

            base += region_pages * TOTAL_MEMORY_PAGE_SIZE;
            total_pages -= region_pages;
        }
    }

    Log(LogLevel::INFO, "PMM: Initialized with success");
    pmm.log_memory_status();

    this->is_initialized = true;
}

void* MemoryManager::Kernel_Alloc(LibC::size_t size, LibC::uint64_t flags)
{
    auto& pmm = PhysicalMemoryManager::instance();
    auto& vmm = VirtualMemoryManager::instance();

    if (!this->is_initialized) {
        FK::panic("MemoryManager: Kernel_Alloc called before initialization");
    }

    if (size == 0) {
        Log(LogLevel::WARN, "MemoryManager: Kernel_Alloc called with size 0");
        return nullptr;
    }

    LibC::size_t page_count = (size + TOTAL_MEMORY_PAGE_SIZE - 1) / TOTAL_MEMORY_PAGE_SIZE;

    LibC::uintptr_t virt_base = vmm.allocate_virtual_range(page_count);
    if (virt_base == 0) {
        Logf(LogLevel::WARN, "MemoryManager: Failed to allocate virtual range for %zu pages", page_count);
        return nullptr;
    }

    for (LibC::size_t i = 0; i < page_count; ++i) {
        LibC::uintptr_t phys = pmm.alloc_page();
        if (phys == 0) {
            Logf(LogLevel::WARN, "MemoryManager: Failed to allocate physical page %zu/%zu", i + 1, page_count);
            for (LibC::size_t j = 0; j < i; ++j) {
                LibC::uintptr_t virt = virt_base + j * TOTAL_MEMORY_PAGE_SIZE;
                LibC::uintptr_t mapped_phys = vmm.translate(virt);
                if (mapped_phys)
                    pmm.free_page(mapped_phys);
                vmm.unmap_page(virt);
            }
            return nullptr;
        }

        bool mapped = vmm.map_page(virt_base + i * TOTAL_MEMORY_PAGE_SIZE, phys, flags | PAGE_PRESENT);
        if (!mapped) {
            Logf(LogLevel::WARN, "MemoryManager: Failed to map virt 0x%p -> phys 0x%p", virt_base + i * TOTAL_MEMORY_PAGE_SIZE, phys);
            pmm.free_page(phys);
            for (LibC::size_t j = 0; j < i; ++j) {
                LibC::uintptr_t virt = virt_base + j * TOTAL_MEMORY_PAGE_SIZE;
                LibC::uintptr_t mapped_phys = vmm.translate(virt);
                if (mapped_phys)
                    pmm.free_page(mapped_phys);
                vmm.unmap_page(virt);
            }
            return nullptr;
        }
    }

    return reinterpret_cast<void*>(virt_base);
}

void MemoryManager::Kernel_Free(void* ptr, LibC::size_t size)
{
    auto& pmm = PhysicalMemoryManager::instance();
    auto& vmm = VirtualMemoryManager::instance();

    if (!this->is_initialized) {
        return;
    }

    if (!ptr || size == 0) {
        Log(LogLevel::WARN, "MemoryManager: Kernel_Free called with invalid arguments");
        return;
    }

    LibC::uintptr_t virt = reinterpret_cast<LibC::uintptr_t>(ptr);
    LibC::size_t page_count = (size + TOTAL_MEMORY_PAGE_SIZE - 1) / TOTAL_MEMORY_PAGE_SIZE;

    for (LibC::size_t i = 0; i < page_count; ++i) {
        LibC::uintptr_t vaddr = virt + i * TOTAL_MEMORY_PAGE_SIZE;
        LibC::uintptr_t paddr = vmm.translate(vaddr);
        if (paddr)
            pmm.free_page(paddr);
        vmm.unmap_page(vaddr);
    }
}

} // namespace MemoryManagement
