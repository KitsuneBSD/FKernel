#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stdint.h>
#include <LibFK/log.h>

#ifdef __x86_64__
#    include <Kernel/Arch/x86_64/Cpu/Constants.h>
#endif // DEBUG

namespace MemoryManagement {

void VirtualMemoryManager::initialize() noexcept
{
    if (pml4 != nullptr) {
        Log(LogLevel::WARN, "VMM: Already initialized");
        return;
    }

    pml4 = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
    if (!pml4) {
        Log(LogLevel::ERROR, "VMM: Failed to allocate PML4");
        return;
    }

    for (LibC::size_t i = 0; i < ENTRIES_PER_PAGE; ++i)
        pml4[i] = 0;

    Log(LogLevel::INFO, "VMM: PML4 initialized");
}

LibC::uint64_t* VirtualMemoryManager::get_or_create_pte(LibC::uintptr_t virt_addr) noexcept
{
    if (!pml4) {
        Log(LogLevel::ERROR, "VMM: PML4 not initialized");
        return nullptr;
    }

    constexpr LibC::uintptr_t mask_9bits = 0x1FF;

    LibC::size_t pml4_idx = (virt_addr >> 39) & mask_9bits;
    LibC::size_t pdpt_idx = (virt_addr >> 30) & mask_9bits;
    LibC::size_t pd_idx = (virt_addr >> 21) & mask_9bits;
    LibC::size_t pt_idx = (virt_addr >> 12) & mask_9bits;

    LibC::uint64_t* pdpt;
    if (pml4[pml4_idx] & PAGE_PRESENT) {
        pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & 0x000FFFFFFFFFF000ULL);
    } else {
        pdpt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE));
        if (!pdpt) {
            Log(LogLevel::ERROR, "VMM: Failed to allocate PDPT");
            return nullptr;
        }
        for (LibC::size_t i = 0; i < ENTRIES_PER_PAGE; ++i)
            pdpt[i] = 0;
        pml4[pml4_idx] = reinterpret_cast<LibC::uintptr_t>(pdpt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PDPT at index %zu for virt addr 0x%llX", pml4_idx, virt_addr);
    }

    LibC::uint64_t* pd;
    if (pdpt[pdpt_idx] & PAGE_PRESENT) {
        pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000ULL);
    } else {
        pd = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (!pd) {
            Log(LogLevel::ERROR, "VMM: Failed to allocate PD");
            return nullptr;
        }
        for (LibC::size_t i = 0; i < ENTRIES_PER_PAGE; ++i)
            pd[i] = 0;
        pdpt[pdpt_idx] = reinterpret_cast<LibC::uintptr_t>(pd) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PD at index %zu for virt addr 0x%llX", pdpt_idx, virt_addr);
    }

    LibC::uint64_t* pt;
    if (pd[pd_idx] & PAGE_PRESENT) {
        pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & 0x000FFFFFFFFFF000ULL);
    } else {
        pt = reinterpret_cast<LibC::uint64_t*>(Falloc(TOTAL_MEMORY_PAGE_SIZE, TOTAL_MEMORY_PAGE_SIZE));
        if (!pt) {
            Log(LogLevel::ERROR, "VMM: Failed to allocate PT");
            return nullptr;
        }
        for (LibC::size_t i = 0; i < ENTRIES_PER_PAGE; ++i)
            pt[i] = 0;
        pd[pd_idx] = reinterpret_cast<LibC::uintptr_t>(pt) | PAGE_PRESENT | PAGE_RW;
        Logf(LogLevel::TRACE, "VMM: Created PT at index %zu for virt addr 0x%llX", pd_idx, virt_addr);
    }

    return &pt[pt_idx];
}

bool VirtualMemoryManager::map_page(LibC::uintptr_t virt_addr, LibC::uintptr_t phys_addr, LibC::uint64_t flags) noexcept
{
    constexpr LibC::uintptr_t page_mask = ~0xFFFULL;

    // Validar alinhamento dos endereços (4 KiB)
    if ((virt_addr & 0xFFF) != 0 || (phys_addr & 0xFFF) != 0) {
        Logf(LogLevel::ERROR, "VMM: map_page chamado com endereços desalinhados virt=0x%llX phys=0x%llX", virt_addr, phys_addr);
        return false;
    }

    virt_addr &= page_mask;
    phys_addr &= page_mask;

    auto* pte = get_or_create_pte(virt_addr);
    if (!pte) {
        Log(LogLevel::ERROR, "VMM: Falha ao obter/criar PTE");
        return false;
    }

    if ((*pte & PAGE_PRESENT) != 0) {
        Logf(LogLevel::WARN, "VMM: Página já mapeada em virt addr 0x%llX", virt_addr);
        return false;
    }

    *pte = (phys_addr & 0x000FFFFFFFFFF000ULL) | (flags | PAGE_PRESENT);

    Logf(LogLevel::INFO, "VMM: Mapeou virt addr 0x%llX para phys addr 0x%llX com flags 0x%llX (PTE=0x%llX)",
        virt_addr, phys_addr, flags, *pte);

    return true;
}

bool VirtualMemoryManager::unmap_page(LibC::uintptr_t virt_addr) noexcept
{
    if (!pml4) {
        Log(LogLevel::ERROR, "VMM: PML4 not initialized");
        return false;
    }

    constexpr LibC::uintptr_t mask_9bits = 0x1FF;
    LibC::size_t pml4_idx = (virt_addr >> 39) & mask_9bits;
    LibC::size_t pdpt_idx = (virt_addr >> 30) & mask_9bits;
    LibC::size_t pd_idx = (virt_addr >> 21) & mask_9bits;
    LibC::size_t pt_idx = (virt_addr >> 12) & mask_9bits;

    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: PML4 entry not present for virt addr 0x%llX", virt_addr);
        return false;
    }
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: PDPT entry not present for virt addr 0x%llX", virt_addr);
        return false;
    }
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: PD entry not present for virt addr 0x%llX", virt_addr);
        return false;
    }
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & 0x000FFFFFFFFFF000ULL);

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
    if (!pml4) {
        Log(LogLevel::ERROR, "VMM: PML4 not initialized");
        return 0;
    }

    constexpr LibC::uintptr_t mask_9bits = 0x1FF;
    LibC::size_t pml4_idx = (virt_addr >> 39) & mask_9bits;
    LibC::size_t pdpt_idx = (virt_addr >> 30) & mask_9bits;
    LibC::size_t pd_idx = (virt_addr >> 21) & mask_9bits;
    LibC::size_t pt_idx = (virt_addr >> 12) & mask_9bits;
    LibC::size_t offset = virt_addr & 0xFFF;

    if (!(pml4[pml4_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PML4 for virt addr 0x%llX", virt_addr);
        return 0;
    }
    auto* pdpt = reinterpret_cast<LibC::uint64_t*>(pml4[pml4_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pdpt[pdpt_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PDPT for virt addr 0x%llX", virt_addr);
        return 0;
    }
    auto* pd = reinterpret_cast<LibC::uint64_t*>(pdpt[pdpt_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pd[pd_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PD for virt addr 0x%llX", virt_addr);
        return 0;
    }
    auto* pt = reinterpret_cast<LibC::uint64_t*>(pd[pd_idx] & 0x000FFFFFFFFFF000ULL);

    if (!(pt[pt_idx] & PAGE_PRESENT)) {
        Logf(LogLevel::WARN, "VMM: Translate failed at PT for virt addr 0x%llX", virt_addr);
        return 0;
    }

    LibC::uintptr_t phys_page = pt[pt_idx] & 0x000FFFFFFFFFF000ULL;
    return phys_page + offset;
}

}
