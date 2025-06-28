#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>

namespace MemoryManagement {

class PhysicalMemoryManager {
private:
    static LibC::uint8_t* pmm_bitmap;
    static LibC::size_t pmm_bitmap_size;
    static LibC::uint64_t pmm_total_pages;
    static LibC::uint64_t pmm_base_addr;
    static LibC::uint64_t total_page_size;

private:
    static void set_bit(LibC::size_t index) noexcept;
    static void clear_bit(LibC::size_t index) noexcept;
    static bool get_bit(LibC::size_t index) noexcept;

public:
    static void initialize(multiboot2::TagMemoryMap const& mmap) noexcept;

    static LibC::uint64_t alloc_page() noexcept;

    static void free_page(LibC::uint64_t phys_addr) noexcept;

    static void mark_region_used(LibC::uint64_t base, LibC::uint64_t size) noexcept;

    static void mark_region_free(LibC::uint64_t base, LibC::uint64_t size) noexcept;
};

}
