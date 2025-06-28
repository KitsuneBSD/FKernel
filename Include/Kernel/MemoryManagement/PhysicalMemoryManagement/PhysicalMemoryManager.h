#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>

namespace MemoryManagement {

class PhysicalMemoryManager {
private:
    LibC::uint8_t* pmm_bitmap;
    LibC::size_t pmm_bitmap_size;
    LibC::uint64_t pmm_total_pages;
    LibC::uint64_t pmm_base_addr;
    LibC::uint64_t total_page_size;

private:
    void set_bit(LibC::size_t index) noexcept;
    void clear_bit(LibC::size_t index) noexcept;
    bool get_bit(LibC::size_t index) noexcept;
    PhysicalMemoryManager() = default;

    void mark_region_used(LibC::uint64_t base, LibC::uint64_t size) noexcept;

    void mark_region_free(LibC::uint64_t base, LibC::uint64_t size) noexcept;

public:
    static PhysicalMemoryManager& instance() noexcept
    {

        static PhysicalMemoryManager s_instance;
        return s_instance;
    }

    void initialize(multiboot2::TagMemoryMap const& mmap) noexcept;

    LibC::uint64_t alloc_page() noexcept;

    void free_page(LibC::uint64_t phys_addr) noexcept;
};

}
