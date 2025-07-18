#include "LibC/string.h"
#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif // DEBUG

namespace MemoryManagement {

static constexpr LibC::size_t ENTRIES_PER_PAGE = 512;
static constexpr LibC::uintptr_t PAGE_MASK = 0x000FFFFFFFFFF000ULL;
static constexpr LibC::uintptr_t ADDR_MASK = 0x000FFFFFFFFFF000ULL;
static constexpr LibC::uintptr_t MASK_BITS = 0x1FF;
static constexpr LibC::uintptr_t TOTAL_MEMORY_PAGE_SIZE = 4096ULL;

void VirtualMemoryManager::initialize() noexcept
{
    FK::enforcef(pml4 == nullptr, "VMM: initialize called but PML4 already initialized");

    pml4 = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
    FK::enforcef(pml4 != nullptr, "VMM: Failed to allocate PML4");

    LibC::memset(pml4, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));

    Log(LogLevel::INFO, "VMM: PML4 initialized");
}

LibC::uint64_t* VirtualMemoryManager::get_or_create_pte(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: get_or_create_pte called but PML4 is nullptr");
    FK::enforcef(virt_addr < (1ULL << 48), "VMM: get_or_create_pte called with virt_addr beyond 48 bits: 0x%llX", virt_addr);
    FK::enforcef((virt_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) == 0, "VMM: get_or_create_pte called with unaligned virt_addr 0x%llX", virt_addr);

    LibC::size_t const pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t const pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t const pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t const pt_idx = (virt_addr >> 12) & MASK_BITS;

    // PML4 level
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        auto* pdpt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        FK::enforcef(pdpt != nullptr, "VMM: Failed to allocate PDPT");
        LibC::memset(pdpt, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
        pml4[pml4_idx] = reinterpret_cast<LibC::uintptr_t>(pdpt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PDPT at index %zu for virt addr 0x%llX", pml4_idx, virt_addr);
    }
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    // PDPT level
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        auto* pd = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        FK::enforcef(pd != nullptr, "VMM: Failed to allocate PD");
        LibC::memset(pd, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
        pdpt[pdpt_idx] = reinterpret_cast<LibC::uintptr_t>(pd) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PD at index %zu for virt addr 0x%llX", pdpt_idx, virt_addr);
    }
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    // PD level
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        auto* pt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        FK::enforcef(pt != nullptr, "VMM: Failed to allocate PT");
        LibC::memset(pt, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
        pd[pd_idx] = reinterpret_cast<LibC::uintptr_t>(pt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PT at index %zu for virt addr 0x%llX", pd_idx, virt_addr);
    }
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    return &pt[pt_idx];
}

bool VirtualMemoryManager::map_page(LibC::uintptr_t virt_addr, LibC::uintptr_t phys_addr, LibC::uint64_t flags) noexcept
{
    FK::enforcef((virt_addr & 0xFFF) == 0, "VMM: map_page called with unaligned virt_addr 0x%llX", virt_addr);
    FK::enforcef((phys_addr & 0xFFF) == 0, "VMM: map_page called with unaligned phys_addr 0x%llX", phys_addr);

    virt_addr &= PAGE_MASK;
    phys_addr &= PAGE_MASK;

    auto* pte = get_or_create_pte(virt_addr);
    FK::enforcef(pte != nullptr, "VMM: Failed to get or create PTE");

    FK::enforcef((*pte & PAGE_PRESENT) == 0, "VMM: Page already mapped at virt addr 0x%llX", virt_addr);

    *pte = (phys_addr & ADDR_MASK) | (flags | PAGE_PRESENT);

    Logf(LogLevel::INFO, "VMM: Mapped virt addr 0x%llX to phys addr 0x%llX with flags 0x%llX (PTE=0x%llX)",
        virt_addr, phys_addr, flags, *pte);

    return true;
}

bool VirtualMemoryManager::unmap_page(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: unmap_page called but PML4 not initialized");
    FK::enforcef((virt_addr & 0xFFF) == 0, "VMM: unmap_page called with unaligned virt_addr 0x%llX", virt_addr);

    LibC::size_t const pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t const pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t const pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t const pt_idx = (virt_addr >> 12) & MASK_BITS;

    FK::enforcef(pml4[pml4_idx] & PAGE_PRESENT, "VMM: PML4 entry not present for virt addr 0x%llX", virt_addr);
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    FK::enforcef(pdpt[pdpt_idx] & PAGE_PRESENT, "VMM: PDPT entry not present for virt addr 0x%llX", virt_addr);
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    FK::enforcef(pd[pd_idx] & PAGE_PRESENT, "VMM: PD entry not present for virt addr 0x%llX", virt_addr);
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    FK::enforcef(pt[pt_idx] & PAGE_PRESENT, "VMM: PT entry not present for virt addr 0x%llX", virt_addr);

    pt[pt_idx] = 0;

    Logf(LogLevel::INFO, "VMM: Unmapped virt addr 0x%llX", virt_addr);
    return true;
}

LibC::uintptr_t VirtualMemoryManager::translate(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: translate called but PML4 not initialized");
    FK::enforcef(virt_addr < (1ULL << 48), "VMM: translate called with virt_addr beyond 48 bits: 0x%llX", virt_addr);

    LibC::size_t const pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t const pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t const pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t const pt_idx = (virt_addr >> 12) & MASK_BITS;
    LibC::size_t const offset = virt_addr & 0xFFF;

    FK::enforcef(pml4[pml4_idx] & PAGE_PRESENT, "VMM: Translate failed at PML4 for virt addr 0x%llX", virt_addr);
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    FK::enforcef(pdpt[pdpt_idx] & PAGE_PRESENT, "VMM: Translate failed at PDPT for virt addr 0x%llX", virt_addr);
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    FK::enforcef(pd[pd_idx] & PAGE_PRESENT, "VMM: Translate failed at PD for virt addr 0x%llX", virt_addr);
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    FK::enforcef(pt[pt_idx] & PAGE_PRESENT, "VMM: Translate failed at PT for virt addr 0x%llX", virt_addr);

    LibC::uintptr_t phys_page = pt[pt_idx] & ADDR_MASK;
    return phys_page + offset;
}

} // namespace MemoryManagement
