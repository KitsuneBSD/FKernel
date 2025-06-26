#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/bump_alloc.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

extern LibC::uintptr_t __heap_start;
extern LibC::uintptr_t __heap_end;

namespace MemoryManagement {

LibC::uint8_t* PhysicalMemoryManager::pmm_bitmap = nullptr;
LibC::size_t PhysicalMemoryManager::pmm_bitmap_size = 0;
LibC::uint64_t PhysicalMemoryManager::pmm_total_pages = 0;
LibC::uint64_t PhysicalMemoryManager::pmm_base_addr = 0;
LibC::uint64_t PhysicalMemoryManager::total_page_size = 4096;

void PhysicalMemoryManager::initialize(multiboot2::TagMemoryMap const& mmap) noexcept
{
    BumpAllocator::initialize(reinterpret_cast<LibC::uintptr_t>(&__heap_start), reinterpret_cast<LibC::uintptr_t>(&__heap_end));
    Log(LogLevel::INFO, "PMM: Initializing Physical Memory Manager");

    LibC::uint64_t lowest_addr = ~0ULL;
    LibC::uint64_t highest_addr = 0;
    LibC::uint64_t free_pages = 0;
    for (auto it = mmap.begin(); it < mmap.end(); ++it) {
        auto const& entry = *it;
        Logf(LogLevel::TRACE, "PMM: Memory region: base=0x%016llX size=0x%016llX type=%u",
            entry.base_addr, entry.length, static_cast<LibC::uint64_t>(entry.type));

        if (entry.type == 1 /* Available */) {
            if (entry.base_addr < lowest_addr)
                lowest_addr = entry.base_addr;
            if (entry.base_addr + entry.length > highest_addr)
                highest_addr = entry.base_addr + entry.length;
        }
    }

    if (lowest_addr >= highest_addr) {
        Log(LogLevel::ERROR, "PMM: No available memory regions found");
        return;
    }

    pmm_base_addr = lowest_addr;
    pmm_total_pages = (highest_addr - pmm_base_addr) / total_page_size;
    pmm_bitmap_size = (pmm_total_pages + 7) / 8;

    Logf(LogLevel::TRACE, "PMM: Base address = 0x%016llX", pmm_base_addr);
    Logf(LogLevel::TRACE, "PMM: Total pages = %llu", pmm_total_pages);
    Logf(LogLevel::TRACE, "PMM: Bitmap size = %zu bytes", pmm_bitmap_size);

    pmm_bitmap = static_cast<LibC::uint8_t*>(BumpAllocator::alloc(pmm_bitmap_size, 8));
    if (!pmm_bitmap) {
        Log(LogLevel::ERROR, "PMM: Failed to allocate bitmap memory");
        return;
    }

    for (LibC::size_t i = 0; i < pmm_bitmap_size; ++i) {
        pmm_bitmap[i] = 0xFF;
    }

    for (auto it = mmap.begin(); it < mmap.end(); ++it) {
        auto const& entry = *it;
        if (entry.type == 1 /* Available */) {
            mark_region_free(entry.base_addr, entry.length);
        } else {
            mark_region_used(entry.base_addr, entry.length);
        }
    }

    for (LibC::uint64_t page = 0; page < pmm_total_pages; ++page) {
        if (!get_bit(page))
            ++free_pages;
    }

    LibC::uint64_t total_memory = pmm_total_pages * total_page_size;

    LibC::uint64_t free_memory = free_pages * total_page_size;

    LibC::uint64_t total_kib = total_memory / 1024;
    LibC::uint64_t total_mib = total_kib / 1024;

    LibC::uint64_t free_kib = free_memory / 1024;
    LibC::uint64_t free_mib = free_kib / 1024;

    Logf(LogLevel::INFO,
        "PMM: Physical memory: total = %llu KiB (%llu MiB), free = %llu KiB (%llu MiB)",
        total_kib, total_mib, free_kib, free_mib);
    Log(LogLevel::INFO, "PMM: Initialization complete");
}

void PhysicalMemoryManager::mark_region_free(LibC::uint64_t base, LibC::uint64_t size) noexcept
{
    LibC::uint64_t start_page = (base < pmm_base_addr) ? 0 : (base - pmm_base_addr) / total_page_size;
    if (start_page > pmm_total_pages)
        start_page = pmm_total_pages;

    LibC::uint64_t end_page = ((base + size + total_page_size - 1) - pmm_base_addr) / total_page_size;
    if (end_page > pmm_total_pages)
        end_page = pmm_total_pages;

    if (end_page > pmm_total_pages)
        end_page = pmm_total_pages;

    Logf(LogLevel::TRACE, "PMM: mark_region_free base=0x%016llX size=0x%016llX start_page=%llu end_page=%llu",
        base, size, start_page, end_page);

    for (LibC::uint64_t page = start_page; page < end_page; ++page) {
        clear_bit(page);
    }
}

void PhysicalMemoryManager::mark_region_used(LibC::uint64_t base, LibC::uint64_t size) noexcept
{
    LibC::uint64_t start_page = (base < pmm_base_addr) ? 0 : (base - pmm_base_addr) / total_page_size;
    if (start_page > pmm_total_pages)
        start_page = pmm_total_pages;

    LibC::uint64_t end_page = ((base + size + total_page_size - 1) - pmm_base_addr) / total_page_size;
    if (end_page > pmm_total_pages)
        end_page = pmm_total_pages;

    if (end_page > pmm_total_pages)
        end_page = pmm_total_pages;

    if (end_page > pmm_total_pages)
        end_page = pmm_total_pages;

    Logf(LogLevel::TRACE, "PMM: mark_region_used base=0x%016llX size=0x%016llX start_page=%llu end_page=%llu",
        base, size, start_page, end_page);

    for (LibC::uint64_t page = start_page; page < end_page; ++page) {
        set_bit(page);
        Logf(LogLevel::TRACE, "PMM: Using page %llu", page);
    }
}

void PhysicalMemoryManager::set_bit(LibC::size_t index) noexcept
{
    if (index >= pmm_total_pages)
        return;
    pmm_bitmap[index / 8] |= (1 << (index % 8));
}

void PhysicalMemoryManager::clear_bit(LibC::size_t index) noexcept
{
    if (index >= pmm_total_pages)
        return;
    pmm_bitmap[index / 8] &= ~(1 << (index % 8));
}

bool PhysicalMemoryManager::get_bit(LibC::size_t index) noexcept
{
    return pmm_bitmap[index / 8] & (1 << (index % 8));
}
}
