#pragma once

#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/FreeBlocks.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

namespace MemoryManagement {

struct PhysicalMemoryRegion {
    LibC::uintptr_t base_addr = 0;
    LibC::uint64_t page_count = 0;
    LibC::uint64_t* bitmap = nullptr;
    LibC::size_t bitmap_size = 0;

    FreeBlock free_block { 0, 0 };

    PhysicalMemoryRegion* next = nullptr;
    PhysicalMemoryRegion* prev = nullptr;

    PhysicalMemoryRegion() noexcept = default;

    PhysicalMemoryRegion(LibC::uintptr_t base, LibC::uint64_t pages) noexcept
        : base_addr(base)
        , page_count(pages)
        , free_block { 0, pages }
    {
    }

    ~PhysicalMemoryRegion() noexcept
    {
        destroy();
    }

    void init(LibC::uintptr_t base, LibC::uint64_t pages) noexcept
    {
        if (bitmap) {
            Ffree(bitmap);
            bitmap = nullptr;
            bitmap_size = 0;
        }

        base_addr = base;
        page_count = pages;
        free_block.start_page = 0;
        free_block.page_count = pages;

        bitmap_size = (pages + 63) / 64;
        bitmap = static_cast<LibC::uint64_t*>(
            Falloc_zeroed(bitmap_size * sizeof(LibC::uint64_t)));
        if (!bitmap) {
            bitmap_size = 0;
            Logf(LogLevel::ERROR, "PMM: Failed to allocate bitmap for region base=0x%lx pages=%lu", base_addr, page_count);
        }

        next = nullptr;
        prev = nullptr;
    }

    void destroy() noexcept
    {
        if (bitmap) {
            Ffree(bitmap);
            bitmap = nullptr;
            bitmap_size = 0;
        }
    }
};

}
