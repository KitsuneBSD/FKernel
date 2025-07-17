#include "LibC/string.h"
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif // DEBUG

namespace MemoryManagement {

inline LibC::size_t extract_index(LibC::uintptr_t virt_addr, unsigned shift) noexcept
{
    return (virt_addr >> shift) & MASK_BITS;
}

inline LibC::uint64_t* get_table_entry(LibC::uint64_t entry) noexcept
{
    return reinterpret_cast<LibC::uint64_t*>(entry & ADDR_MASK);
}

inline void zero_table(LibC::uint64_t* table) noexcept
{
    LibC::memset(table, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
}

void VirtualMemoryManager::initialize() noexcept
{
    FK::enforcef(pml4 == nullptr, "VMM: initialize called but PML4 already initialized");

    pml4 = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
    FK::enforcef(pml4 != nullptr, "VMM: Failed to allocate PML4");

    zero_table(pml4);

    Log(LogLevel::INFO, "VMM: PML4 initialized");
}

LibC::uint64_t* VirtualMemoryManager::get_or_create_pte(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: get_or_create_pte called but PML4 is nullptr");

    LibC::size_t const pml4_idx = extract_index(virt_addr, 39);
    LibC::size_t const pdpt_idx = extract_index(virt_addr, 30);
    LibC::size_t const pd_idx = extract_index(virt_addr, 21);
    LibC::size_t const pt_idx = extract_index(virt_addr, 12);

    LibC::uint64_t* pdpt;
    if (pml4[pml4_idx] & PAGE_PRESENT) {
        pdpt = get_table_entry(pml4[pml4_idx]);
    } else {
        pdpt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (!pdpt) {
            Log(LogLevel::ERROR, "VMM: Failed to allocate PDPT");
            return nullptr;
        }
        zero_table(pdpt);
        pml4[pml4_idx] = reinterpret_cast<LibC::uintptr_t>(pdpt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PDPT at index %zu for virt addr 0x%llX", pml4_idx, virt_addr);
    }

    LibC::uint64_t* pd;
    if (pdpt[pdpt_idx] & PAGE_PRESENT) {
        pd = get_table_entry(pdpt[pdpt_idx]);
    } else {
        pd = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (!pd) {
            Log(LogLevel::ERROR, "VMM: Failed to allocate PD");
            return nullptr;
        }
        zero_table(pd);
        pdpt[pdpt_idx] = reinterpret_cast<LibC::uintptr_t>(pd) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PD at index %zu for virt addr 0x%llX", pdpt_idx, virt_addr);
    }

    LibC::uint64_t* pt;
    if (pd[pd_idx] & PAGE_PRESENT) {
        pt = get_table_entry(pd[pd_idx]);
    } else {
        pt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (!pt) {
            Log(LogLevel::ERROR, "VMM: Failed to allocate PT");
            return nullptr;
        }
        zero_table(pt);
        pd[pd_idx] = reinterpret_cast<LibC::uintptr_t>(pt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PT at index %zu for virt addr 0x%llX", pd_idx, virt_addr);
    }

    return &pt[pt_idx];
}

bool VirtualMemoryManager::map_page(LibC::uintptr_t virt_addr, LibC::uintptr_t phys_addr, LibC::uint64_t flags) noexcept
{
    if ((virt_addr & 0xFFF) != 0 || (phys_addr & 0xFFF) != 0) {
        Logf(LogLevel::ERROR, "VMM: map_page called with unaligned addresses virt=0x%llX phys=0x%llX", virt_addr, phys_addr);
        return false;
    }

    virt_addr &= PAGE_MASK;
    phys_addr &= PAGE_MASK;

    auto* pte = get_or_create_pte(virt_addr);
    if (!pte) {
        Log(LogLevel::ERROR, "VMM: Failed to get or create PTE");
        return false;
    }

    if ((*pte & PAGE_PRESENT) != 0) {
        Logf(LogLevel::WARN, "VMM: Page already mapped at virt addr 0x%llX", virt_addr);
        return false;
    }

    *pte = (phys_addr & ADDR_MASK) | (flags | PAGE_PRESENT);

    Logf(LogLevel::INFO, "VMM: Mapped virt addr 0x%llX to phys addr 0x%llX with flags 0x%llX (PTE=0x%llX)",
        virt_addr, phys_addr, flags, *pte);

    return true;
}

bool VirtualMemoryManager::unmap_page(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: unmap_page called but PML4 not initialized");

    LibC::size_t const pml4_idx = extract_index(virt_addr, 39);
    LibC::size_t const pdpt_idx = extract_index(virt_addr, 30);
    LibC::size_t const pd_idx = extract_index(virt_addr, 21);
    LibC::size_t const pt_idx = extract_index(virt_addr, 12);

    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: PML4 entry not present for virt addr 0x%llX", virt_addr);
        return false;
    }
    auto* pdpt = get_table_entry(pml4[pml4_idx]);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: PDPT entry not present for virt addr 0x%llX", virt_addr);
        return false;
    }
    auto* pd = get_table_entry(pdpt[pdpt_idx]);

    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: PD entry not present for virt addr 0x%llX", virt_addr);
        return false;
    }
    auto* pt = get_table_entry(pd[pd_idx]);

    if (!(pt[pt_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: PT entry not present for virt addr 0x%llX", virt_addr);
        return false;
    }

    pt[pt_idx] = 0;
    Logf(LogLevel::INFO, "VMM: Unmapped virt addr 0x%llX", virt_addr);
    return true;
}

LibC::uintptr_t VirtualMemoryManager::translate(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: translate called but PML4 not initialized");

    LibC::size_t const pml4_idx = extract_index(virt_addr, 39);
    LibC::size_t const pdpt_idx = extract_index(virt_addr, 30);
    LibC::size_t const pd_idx = extract_index(virt_addr, 21);
    LibC::size_t const pt_idx = extract_index(virt_addr, 12);
    LibC::size_t const offset = virt_addr & 0xFFF;

    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PML4 for virt addr 0x%llX", virt_addr);
        return 0;
    }
    auto* pdpt = get_table_entry(pml4[pml4_idx]);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PDPT for virt addr 0x%llX", virt_addr);
        return 0;
    }
    auto* pd = get_table_entry(pdpt[pdpt_idx]);

    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PD for virt addr 0x%llX", virt_addr);
        return 0;
    }
    auto* pt = get_table_entry(pd[pd_idx]);

    if (!(pt[pt_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PT for virt addr 0x%llX", virt_addr);
        return 0;
    }

    LibC::uintptr_t phys_page = pt[pt_idx] & ADDR_MASK;
    return phys_page + offset;
}

}
