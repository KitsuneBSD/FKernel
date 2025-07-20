#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
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

LibC::uint64_t* VirtualMemoryManager::get_or_create_pte(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: get_or_create_pte called but PML4 is nullptr");

    if (FK::alert_if_f(virt_addr >= (1ULL << 48), "VMM: get_or_create_pte called with virt_addr beyond 48 bits: 0x%llX", virt_addr))
        return nullptr;

    if (FK::alert_if_f((virt_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) != 0, "VMM: get_or_create_pte called with unaligned virt_addr 0x%llX", virt_addr))
        return nullptr;

    LibC::size_t const pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t const pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t const pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t const pt_idx = (virt_addr >> 12) & MASK_BITS;

    // PML4 level
    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        auto* pdpt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (FK::alert_if_f(pdpt == nullptr, "VMM: Failed to allocate PDPT"))
            return nullptr;
        LibC::memset(pdpt, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
        pml4[pml4_idx] = reinterpret_cast<LibC::uintptr_t>(pdpt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PDPT at index %zu for virt addr 0x%llX", pml4_idx, virt_addr);
    }
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    // PDPT level
    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        auto* pd = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (FK::alert_if_f(pd == nullptr, "VMM: Failed to allocate PD"))
            return nullptr;
        LibC::memset(pd, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
        pdpt[pdpt_idx] = reinterpret_cast<LibC::uintptr_t>(pd) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PD at index %zu for virt addr 0x%llX", pdpt_idx, virt_addr);
    }
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    // PD level
    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        auto* pt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (FK::alert_if_f(pt == nullptr, "VMM: Failed to allocate PT"))
            return nullptr;
        LibC::memset(pt, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
        pd[pd_idx] = reinterpret_cast<LibC::uintptr_t>(pt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PT at index %zu for virt addr 0x%llX", pd_idx, virt_addr);
    }
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    return &pt[pt_idx];
}

bool VirtualMemoryManager::map_page(LibC::uintptr_t virt_addr, LibC::uintptr_t phys_addr, LibC::uint64_t flags) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: map_page called but PML4 is nullptr");

    if (FK::alert_if_f((virt_addr & 0xFFF) != 0, "VMM: map_page called with unaligned virt_addr 0x%llX", virt_addr))
        return false;

    if (FK::alert_if_f((phys_addr & 0xFFF) != 0, "VMM: map_page called with unaligned phys_addr 0x%llX", phys_addr))
        return false;

    virt_addr &= PAGE_MASK;
    phys_addr &= PAGE_MASK;

    auto* pte = get_or_create_pte(virt_addr);
    if (pte == nullptr)
        return false;

    if (FK::alert_if_f((*pte & PAGE_PRESENT) != 0, "VMM: Page already mapped at virt addr 0x%llX", virt_addr))
        return false;

    *pte = (phys_addr & ADDR_MASK) | (flags | PAGE_PRESENT);

    Logf(LogLevel::TRACE, "VMM: Mapped virt addr 0x%llX to phys addr 0x%llX with flags 0x%llX (PTE=0x%llX)",
        virt_addr, phys_addr, flags, *pte);

    return true;
}

bool VirtualMemoryManager::unmap_page(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: unmap_page called but PML4 not initialized");

    if (FK::alert_if_f((virt_addr & 0xFFF) != 0, "VMM: unmap_page called with unaligned virt_addr 0x%llX", virt_addr))
        return false;

    LibC::size_t const pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t const pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t const pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t const pt_idx = (virt_addr >> 12) & MASK_BITS;

    if (FK::alert_if_f(!(pml4[pml4_idx] & PAGE_PRESENT), "VMM: PML4 entry not present for virt addr 0x%llX", virt_addr))
        return false;
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    if (FK::alert_if_f(!(pdpt[pdpt_idx] & PAGE_PRESENT), "VMM: PDPT entry not present for virt addr 0x%llX", virt_addr))
        return false;
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    if (FK::alert_if_f(!(pd[pd_idx] & PAGE_PRESENT), "VMM: PD entry not present for virt addr 0x%llX", virt_addr))
        return false;
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    if (FK::alert_if_f(!(pt[pt_idx] & PAGE_PRESENT), "VMM: PT entry not present for virt addr 0x%llX", virt_addr))
        return false;

    pt[pt_idx] = 0;

    Logf(LogLevel::TRACE, "VMM: Unmapped virt addr 0x%llX", virt_addr);
    return true;
}

LibC::uintptr_t VirtualMemoryManager::translate(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: translate called but PML4 not initialized");

    if (FK::alert_if_f(virt_addr >= (1ULL << 48), "VMM: translate called with virt_addr beyond 48 bits: 0x%llX", virt_addr))
        return 0;

    LibC::size_t const pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t const pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t const pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t const pt_idx = (virt_addr >> 12) & MASK_BITS;
    LibC::size_t const offset = virt_addr & 0xFFF;

    if (FK::alert_if_f(!(pml4[pml4_idx] & PAGE_PRESENT), "VMM: Translate failed at PML4 for virt addr 0x%llX", virt_addr))
        return 0;
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    if (FK::alert_if_f(!(pdpt[pdpt_idx] & PAGE_PRESENT), "VMM: Translate failed at PDPT for virt addr 0x%llX", virt_addr))
        return 0;
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    if (FK::alert_if_f(!(pd[pd_idx] & PAGE_PRESENT), "VMM: Translate failed at PD for virt addr 0x%llX", virt_addr))
        return 0;
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    if (FK::alert_if_f(!(pt[pt_idx] & PAGE_PRESENT), "VMM: Translate failed at PT for virt addr 0x%llX", virt_addr))
        return 0;

    LibC::uintptr_t phys_page = pt[pt_idx] & ADDR_MASK;
    return phys_page + offset;
}

LibC::uint64_t* VirtualMemoryManager::get_pte(LibC::uintptr_t virt_addr) noexcept
{
    FK::enforcef(pml4 != nullptr, "VMM: get_pte called but PML4 is nullptr");
    FK::enforcef(virt_addr < (1ULL << 48), "VMM: get_pte called with virt_addr beyond 48 bits: 0x%llX", virt_addr);
    FK::enforcef((virt_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) == 0, "VMM: get_pte called with unaligned virt_addr 0x%llX", virt_addr);

    LibC::size_t const pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t const pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t const pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t const pt_idx = (virt_addr >> 12) & MASK_BITS;

    if (!(pml4[pml4_idx] & PAGE_PRESENT))
        return nullptr;
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT))
        return nullptr;
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    if (!(pd[pd_idx] & PAGE_PRESENT))
        return nullptr;
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    if (!(pt[pt_idx] & PAGE_PRESENT))
        return nullptr;

    return &pt[pt_idx];
}

bool VirtualMemoryManager::is_mapped(LibC::uintptr_t virt_addr) noexcept
{
    auto* pte = get_pte(virt_addr);
    return (pte != nullptr);
}

} // namespace MemoryManagement
