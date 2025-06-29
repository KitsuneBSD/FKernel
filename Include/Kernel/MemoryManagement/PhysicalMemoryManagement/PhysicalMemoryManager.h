#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/FreeBlocks.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <LibC/stdint.h>

namespace MemoryManagement {
class PhysicalMemoryManager {
private:
    LibC::uint64_t* pmm_bitmap;     // Bitmap de páginas, 64 bits por palavra
    LibC::size_t pmm_bitmap_size;   // Número de palavras de 64 bits no bitmap
    LibC::uint64_t pmm_total_pages; // Total de páginas físicas gerenciadas
    LibC::uint64_t pmm_base_addr;   // Endereço base da memória gerenciada
    LibC::uint64_t total_page_size; // Tamanho fixo da página (ex: 4096 bytes)

    static constexpr LibC::size_t max_free_blocks = 1024;
    FreeBlock free_blocks[max_free_blocks];
    LibC::size_t free_block_count = 0;
    bool bitmap_dirty = false;

private:
    PhysicalMemoryManager() = default;

    void set_bit(LibC::size_t index) noexcept;
    void clear_bit(LibC::size_t index) noexcept;
    bool get_bit(LibC::size_t index) noexcept;

    void mark_region_used(LibC::uint64_t base, LibC::uint64_t size) noexcept;
    void mark_region_free(LibC::uint64_t base, LibC::uint64_t size) noexcept;

    void add_free_block(LibC::uint64_t start_page, LibC::uint64_t page_count) noexcept;
    void consolidate_blocks() noexcept;
    void sync_free_blocks_with_bitmap() noexcept;
    void sync_free_blocks_if_needed() noexcept;

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

    LibC::uint64_t total_pages() const noexcept { return pmm_total_pages; }
    LibC::uint64_t total_bytes() const noexcept { return pmm_total_pages * total_page_size; }
    LibC::uint64_t base_address() const noexcept { return pmm_base_addr; }
};
}
