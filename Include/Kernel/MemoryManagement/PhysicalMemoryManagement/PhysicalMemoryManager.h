#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryRegion.h>
#include <LibC/stdint.h>

namespace MemoryManagement {
class PhysicalMemoryManager {
private:
    PhysicalMemoryRegion* region_list_head = nullptr;
    PhysicalMemoryRegion* region_list_tail = nullptr;

    void add_region(PhysicalMemoryRegion* region) noexcept;
    PhysicalMemoryRegion* find_region(LibC::uintptr_t phys_addr) noexcept;
    void remove_region(LibC::uintptr_t phys_addr) noexcept;
    LibC::size_t count_used_pages() const noexcept;
    LibC::uint64_t total_pages() const noexcept;
    LibC::uint64_t free_pages() const noexcept;
    LibC::uint64_t total_bytes() const noexcept;
    LibC::uint64_t free_bytes() const noexcept;

    LibC::size_t region_count() const noexcept;
    LibC::size_t allocated_region_count() const noexcept;

    void ensure_bitmap_allocated(PhysicalMemoryRegion* region) noexcept;
    bool find_free_pages_in_region(PhysicalMemoryRegion* region, LibC::uint64_t count, LibC::uint64_t& out_start_page) noexcept;
    void mark_pages(PhysicalMemoryRegion* region, LibC::uint64_t start_page, LibC::uint64_t count, bool allocate) noexcept;

public:
    static PhysicalMemoryManager& instance() noexcept
    {
        static PhysicalMemoryManager s_instance;
        return s_instance;
    }

    void initialize(multiboot2::TagMemoryMap const& mmap) noexcept;
    LibC::uintptr_t alloc_page() noexcept;
    void free_page(LibC::uintptr_t phys_addr) noexcept;
    LibC::uintptr_t alloc_contiguous_pages(LibC::uint64_t page_count) noexcept;
    void free_contiguous_pages(LibC::uintptr_t phys_addr, LibC::uint64_t page_count) noexcept;
    void log_memory_status() const noexcept;
};
}
