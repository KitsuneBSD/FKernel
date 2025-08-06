#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/log.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Asm.h>
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif // DEBUG

namespace MemoryManagement {

static constexpr LibC::uintptr_t KERNEL_PHYS_OFFSET = 0xFFFF800000000000ULL;
static constexpr LibC::size_t ENTRIES_PER_PAGE = 512;
static constexpr LibC::uintptr_t PAGE_SIZE = 0x1000;
static constexpr LibC::uintptr_t PAGE_MASK = ~(PAGE_SIZE - 1);
static constexpr LibC::uintptr_t ADDR_MASK = ~0xFFFULL;
static constexpr LibC::uintptr_t INDEX_MASK = 0x1FF;
static constexpr LibC::uintptr_t CANONICAL_MASK = 0xFFFF000000000000ULL;

bool VirtualMemoryManager::is_canonical(LibC::uintptr_t addr) noexcept
{
    constexpr LibC::uintptr_t sign_bit = 1ULL << 47;
    if ((addr & sign_bit) == 0) {
        return (addr & ~((1ULL << 48) - 1)) == 0;
    } else {
        return (addr & CANONICAL_MASK) == CANONICAL_MASK;
    }
}

[[nodiscard]] inline void* phys_to_virt(LibC::uintptr_t phys_addr) noexcept
{
    return reinterpret_cast<void*>(phys_addr + KERNEL_PHYS_OFFSET);
}

LibC::uint64_t* VirtualMemoryManager::get_or_create_pte(LibC::uintptr_t virt_addr) noexcept
{
    if (pml4 == nullptr) {
        Log(LogLevel::ERROR, "VMM: get_or_create_pte called but pml4 is nullptr");
        return nullptr;
    }

    if (!is_canonical(virt_addr)) {
        Logf(LogLevel::ERROR, "VMM: get_or_create_pte virt_addr out of range: 0x%llX", virt_addr);
        return nullptr;
    }

    if ((virt_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) != 0) {
        Logf(LogLevel::ERROR, "VMM: get_or_create_pte virt_addr not page aligned: 0x%llX", virt_addr);
        return nullptr;
    }

    constexpr LibC::uintptr_t page_size = TOTAL_MEMORY_PAGE_SIZE;
    LibC::size_t pml4_idx = (virt_addr >> 39) & MASK_BITS;
    LibC::size_t pdpt_idx = (virt_addr >> 30) & MASK_BITS;
    LibC::size_t pd_idx = (virt_addr >> 21) & MASK_BITS;
    LibC::size_t pt_idx = (virt_addr >> 12) & MASK_BITS;

    auto alloc_page_table = [](char const* level_name) -> LibC::uint64_t* {
        LibC::uint64_t* table = reinterpret_cast<LibC::uint64_t*>(Falloc(page_size, page_size));
        if (table == nullptr) {
            Logf(LogLevel::ERROR, "VMM: Failed to allocate %s table", level_name);
            return nullptr;
        }
        LibC::memset(table, 0, ENTRIES_PER_PAGE * sizeof(LibC::uint64_t));
        return table;
    };

    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        auto* pdpt = alloc_page_table("PDPT");
        if (pdpt == nullptr)
            return nullptr;
        pml4[pml4_idx] = reinterpret_cast<LibC::uintptr_t>(pdpt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PDPT at PML4 index %zu for virt addr 0x%llX", pml4_idx, virt_addr);
    }
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(phys_to_virt(pml4[pml4_idx] & PAGE_MASK));
    if (pdpt == nullptr) {
        Logf(LogLevel::ERROR, "VMM: PDPT translation failed for virt addr 0x%llX", virt_addr);
        return nullptr;
    }

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        auto* pd = alloc_page_table("PD");
        if (pd == nullptr)
            return nullptr;
        pdpt[pdpt_idx] = reinterpret_cast<LibC::uintptr_t>(pd) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PD at PDPT index %zu for virt addr 0x%llX", pdpt_idx, virt_addr);
    }
    auto* pd = reinterpret_cast<LibC::uint64_t*>(phys_to_virt(pdpt[pdpt_idx] & PAGE_MASK));
    if (pd == nullptr) {
        Logf(LogLevel::ERROR, "VMM: PD translation failed for virt addr 0x%llX", virt_addr);
        return nullptr;
    }

    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        auto* pt = alloc_page_table("PT");
        if (pt == nullptr)
            return nullptr;
        pd[pd_idx] = reinterpret_cast<LibC::uintptr_t>(pt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PT at PD index %zu for virt addr 0x%llX", pd_idx, virt_addr);
    }
    auto* pt = reinterpret_cast<LibC::uint64_t*>(phys_to_virt(pd[pd_idx] & PAGE_MASK));
    if (pt == nullptr) {
        Logf(LogLevel::ERROR, "VMM: PT translation failed for virt addr 0x%llX", virt_addr);
        return nullptr;
    }

    return &pt[pt_idx];
}

bool VirtualMemoryManager::map_page(LibC::uintptr_t virt_addr, LibC::uintptr_t phys_addr, LibC::uint64_t flags) noexcept
{
    if ((virt_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) != 0) {
        Logf(LogLevel::ERROR, "VMM: map_page virt_addr not aligned: 0x%llX", virt_addr);
        return false;
    }
    if ((phys_addr & (TOTAL_MEMORY_PAGE_SIZE - 1)) != 0) {
        Logf(LogLevel::ERROR, "VMM: map_page phys_addr not aligned: 0x%llX", phys_addr);
        return false;
    }

    auto* pte = get_or_create_pte(virt_addr);
    if (pte == nullptr) {
        Logf(LogLevel::ERROR, "VMM: map_page failed to get or create PTE for virt_addr 0x%llX", virt_addr);
        return false;
    }

    if ((*pte & PAGE_PRESENT) != 0) {
        Logf(LogLevel::WARN, "VMM: map_page virt_addr 0x%llX is already mapped", virt_addr);
        return false;
    }

    *pte = (phys_addr & PAGE_MASK) | (flags | PAGE_PRESENT);

    Logf(LogLevel::INFO, "VMM: Mapped virt addr 0x%llX to phys addr 0x%llX with flags 0x%llX", virt_addr, phys_addr, flags);

    return true;
}

bool VirtualMemoryManager::unmap_page(LibC::uintptr_t virt_addr) noexcept
{
    if (!pml4 || (virt_addr & 0xFFF))
        return false;

    auto const pml4_idx = (virt_addr >> 39) & INDEX_MASK;
    auto const pdpt_idx = (virt_addr >> 30) & INDEX_MASK;
    auto const pd_idx = (virt_addr >> 21) & INDEX_MASK;
    auto const pt_idx = (virt_addr >> 12) & INDEX_MASK;

    if (FK::alert_if_f(!(pml4[pml4_idx] & PAGE_PRESENT), "VMM: No PML4 entry for 0x%llX", virt_addr))
        return false;

    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);
    if (FK::alert_if_f(!(pdpt[pdpt_idx] & PAGE_PRESENT), "VMM: No PDPT entry for 0x%llX", virt_addr))
        return false;

    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);
    if (FK::alert_if_f(!(pd[pd_idx] & PAGE_PRESENT), "VMM: No PD entry for 0x%llX", virt_addr))
        return false;

    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);
    if (FK::alert_if_f(!(pt[pt_idx] & PAGE_PRESENT), "VMM: No PT entry for 0x%llX", virt_addr))
        return false;

    pt[pt_idx] = 0;
    Logf(LogLevel::INFO, "VMM: Unmapped 0x%llX", virt_addr);
    return true;
}

LibC::uintptr_t VirtualMemoryManager::translate(LibC::uintptr_t virt_addr) noexcept
{
    if (!pml4 || !is_canonical(virt_addr))
        return 0;

    auto const pml4_idx = (virt_addr >> 39) & INDEX_MASK;
    auto const pdpt_idx = (virt_addr >> 30) & INDEX_MASK;
    auto const pd_idx = (virt_addr >> 21) & INDEX_MASK;
    auto const pt_idx = (virt_addr >> 12) & INDEX_MASK;
    auto const offset = virt_addr & 0xFFF;

    if (!(pml4[pml4_idx] & PAGE_PRESENT))
        return 0;
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & ADDR_MASK);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT))
        return 0;
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & ADDR_MASK);

    if (!(pd[pd_idx] & PAGE_PRESENT))
        return 0;
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & ADDR_MASK);

    if (!(pt[pt_idx] & PAGE_PRESENT))
        return 0;

    return (pt[pt_idx] & ADDR_MASK) + offset;
}

LibC::uintptr_t VirtualMemoryManager::allocate_virtual_range(LibC::size_t page_count)
{
    if (page_count == 0 || !pml4)
        return 0;

    constexpr LibC::uintptr_t KERNEL_BASE = 0xFFFF800000000000ULL;
    constexpr LibC::uintptr_t MAX_ADDR = 0xFFFFFFFFFFFFF000ULL;

    LibC::uintptr_t candidate = MAX_ADDR - (page_count * PAGE_SIZE);
    while (candidate >= KERNEL_BASE) {
        bool available = true;

        for (LibC::size_t i = 0; i < page_count; ++i) {
            if (is_mapped(candidate + i * PAGE_SIZE)) {
                available = false;
                candidate -= (i + 1) * PAGE_SIZE;
                break;
            }
        }

        if (available)
            return candidate;

        if (candidate < KERNEL_BASE + PAGE_SIZE)
            break;
    }

    Logf(LogLevel::WARN, "VMM: Failed to find virtual range for %zu pages", page_count);
    return 0;
}

bool VirtualMemoryManager::is_mapped(LibC::uintptr_t virt_addr) noexcept
{
    if (!pml4)
        return false;

    auto const pml4_idx = (virt_addr >> 39) & INDEX_MASK;
    auto const pdpt_idx = (virt_addr >> 30) & INDEX_MASK;
    auto const pd_idx = (virt_addr >> 21) & INDEX_MASK;
    auto const pt_idx = (virt_addr >> 12) & INDEX_MASK;

    auto pml4_entry = pml4[pml4_idx];
    if (!(pml4_entry & PAGE_PRESENT))
        return false;

    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(phys_to_virt(pml4_entry & PAGE_MASK));
    auto pdpt_entry = pdpt[pdpt_idx];
    if (!(pdpt_entry & PAGE_PRESENT))
        return false;
    if (pdpt_entry & PAGE_PSE)
        return true;

    auto* pd = reinterpret_cast<LibC::uint64_t*>(phys_to_virt(pdpt_entry & PAGE_MASK));
    auto pd_entry = pd[pd_idx];
    if (!(pd_entry & PAGE_PRESENT))
        return false;
    if (pd_entry & PAGE_PSE)
        return true;

    auto* pt = reinterpret_cast<LibC::uint64_t*>(phys_to_virt(pd_entry & PAGE_MASK));
    return pt[pt_idx] & PAGE_PRESENT;
}

} // namespace MemoryManagement
