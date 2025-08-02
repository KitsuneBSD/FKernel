#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

extern "C" LibC::uint64_t* current_pml4_ptr;

namespace MemoryManagement {

// TODO: Rewrite with builder design pattern
constexpr LibC::uint64_t PAGE_PRESENT = 1ULL << 0;
constexpr LibC::uint64_t PAGE_RW = 1ULL << 1;
constexpr LibC::uint64_t PAGE_USER = 1ULL << 2;
constexpr LibC::uint64_t PAGE_PWT = 1ULL << 3;
constexpr LibC::uint64_t PAGE_PCD = 1ULL << 4;
constexpr LibC::uint64_t PAGE_ACCESSED = 1ULL << 5;
constexpr LibC::uint64_t PAGE_DIRTY = 1ULL << 6;
constexpr LibC::uint64_t PAGE_PSE = 1ULL << 7; // Para páginas 2 MiB
constexpr LibC::uint64_t PAGE_GLOBAL = 1ULL << 8;
constexpr LibC::uint64_t PAGE_NX = 1ULL << 63;

class VirtualMemoryManager {
private:
    VirtualMemoryManager() = default;

    LibC::uintptr_t find_free_virtual_range(LibC::size_t page_count);

public:
    static VirtualMemoryManager& instance() noexcept
    {

        static VirtualMemoryManager v_instance;
        return v_instance;
    }

    LibC::uint64_t* pml4 = current_pml4_ptr;
    void create_tables_if_needed(LibC::uintptr_t virt_addr) noexcept;
    LibC::uint64_t* get_or_create_pte(LibC::uintptr_t virt_addr) noexcept;
    LibC::uint64_t* get_pte(LibC::uintptr_t virt_addr) noexcept;
    bool is_mapped(LibC::uintptr_t virt_addr) noexcept;

    bool map_page(LibC::uintptr_t virt_addr, LibC::uintptr_t phys_addr, LibC::uint64_t flags) noexcept;
    bool unmap_page(LibC::uintptr_t virt_addr) noexcept;
    LibC::uintptr_t translate(LibC::uintptr_t virt_addr) noexcept;
    bool is_canonical(LibC::uintptr_t addr) noexcept;
    LibC::uintptr_t allocate_virtual_range(LibC::size_t page_count);
    LibC::uintptr_t get_physical_address(LibC::uintptr_t virt_addr);
};

}
