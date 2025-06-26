#pragma once

#include <LibC/stdint.h>

namespace MemoryManagement {

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
public:
    static void initialize() noexcept;

    static bool map_page(LibC::uintptr_t virt_addr, LibC::uintptr_t phys_addr, LibC::uint64_t flags) noexcept;
    static bool unmap_page(LibC::uintptr_t virt_addr) noexcept;
    static LibC::uintptr_t translate(LibC::uintptr_t virt_addr) noexcept;

    static void flush_tlb_single(LibC::uintptr_t virt_addr) noexcept;

private:
    static LibC::uint64_t* pml4;
    static void create_tables_if_needed(LibC::uintptr_t virt_addr) noexcept;
    static LibC::uint64_t* get_or_create_pte(LibC::uintptr_t virt_addr) noexcept;
};

}
