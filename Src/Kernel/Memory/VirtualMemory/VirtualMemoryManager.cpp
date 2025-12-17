#include <Kernel/Memory/VirtualMemory/VirtualMemoryManager.h>
#include <Kernel/Memory/PhysicalMemory/PhysicalMemoryManager.h>
#include <LibFK/Algorithms/log.h>

VirtualMemoryManager::VirtualMemoryManager()
    : m_pml4(nullptr), m_pml4_phys(0) {
    fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Ctor (empty)");
}

VirtualMemoryManager& VirtualMemoryManager::the() {
    static VirtualMemoryManager inst;
    return inst;
}

void VirtualMemoryManager::invlpg(uintptr_t addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

// Realiza o mapeamento identidade inicial
void VirtualMemoryManager::perform_initial_identity_mapping() {
    size_t pages = INITIAL_IDENTITY_MAPPING_SIZE / PAGE_SIZE;
    fk::algorithms::klog(
        "VIRTUAL MEMORY MANAGER",
        "Identity mapping start: pages=%zu",
        pages
    );
    
    for (size_t i = 0; i < pages; i++) {
        uintptr_t phys = i * PAGE_SIZE;
        map_page(phys, phys, PageFlags::Present | PageFlags::Writable);
    }
    
    fk::algorithms::klog(
        "VIRTUAL MEMORY MANAGER",
        "Identity mapping done"
    );

}

void VirtualMemoryManager::initialize() {
    if (m_pml4){
        fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Initialize skipped: already initialized");
        return;
    }

    // Aloca PML4 com uma página
    m_pml4_phys = PhysicalMemoryManager::the().alloc_page();
    assert(m_pml4_phys != 0);

    fk::algorithms::kdebug(
        "VIRTUAL MEMORY MANAGER",
        "PML4 allocated: phys=%p",
        m_pml4_phys
    );

    m_pml4 = reinterpret_cast<PageTable*>(m_pml4_phys);
    for (size_t i = 0; i < MAX_TABLES; i++)
        m_pml4->entries[i] = 0;

    perform_initial_identity_mapping();
    write_on_cr3(static_cast<void*>(m_pml4));

    fk::algorithms::klog(
        "VIRTUAL MEMORY MANAGER",
        "Initialize done: cr3=%p",
        m_pml4
    );
}

void VirtualMemoryManager::map_page(uintptr_t virt, uintptr_t phys, PageFlags flags) {
    assert((virt % PAGE_SIZE) == 0);
    assert((phys % PAGE_SIZE) == 0);

    size_t pml4_idx = (virt >> 39) & 0x1FF;
    size_t pdpt_idx = (virt >> 30) & 0x1FF;
    size_t pd_idx   = (virt >> 21) & 0x1FF;
    size_t pt_idx   = (virt >> 12) & 0x1FF;

    // PML4
    if (!(m_pml4->entries[pml4_idx] & static_cast<uint64_t>(PageFlags::Present))) {
        uintptr_t new_pdpt = PhysicalMemoryManager::the().alloc_page();
        assert(new_pdpt != 0);
        m_pml4->entries[pml4_idx] = new_pdpt | static_cast<uint64_t>(PageFlags::Present) | static_cast<uint64_t>(PageFlags::Writable);
    }
    PageTable* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000);

    // PDPT
    if (!(pdpt->entries[pdpt_idx] & static_cast<uint64_t>(PageFlags::Present))) {
        uintptr_t new_pd = PhysicalMemoryManager::the().alloc_page();
        assert(new_pd != 0);
        pdpt->entries[pdpt_idx] = new_pd | static_cast<uint64_t>(PageFlags::Present) | static_cast<uint64_t>(PageFlags::Writable);
    }
    PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000);

    // PD
    if (!(pd->entries[pd_idx] & static_cast<uint64_t>(PageFlags::Present))) {
        uintptr_t new_pt = PhysicalMemoryManager::the().alloc_page();
        assert(new_pt != 0);
        pd->entries[pd_idx] = new_pt | static_cast<uint64_t>(PageFlags::Present) | static_cast<uint64_t>(PageFlags::Writable);
    }
    PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & 0x000FFFFFFFFFF000);

    // PT
    pt->entries[pt_idx] = phys | static_cast<uint64_t>(flags) | static_cast<uint64_t>(PageFlags::Present);
    invlpg(virt);

    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "VIRTUAL MEMORY MANAGER",
        "Map page: virt=%p phys=%p flags=%lx",
        virt,
        phys,
        static_cast<uint64_t>(flags)
    );
    */
}

uintptr_t VirtualMemoryManager::translate(uintptr_t virt) {
    assert((virt % PAGE_SIZE) == 0);

    size_t pml4_idx = (virt >> 39) & 0x1FF;
    size_t pdpt_idx = (virt >> 30) & 0x1FF;
    size_t pd_idx   = (virt >> 21) & 0x1FF;
    size_t pt_idx   = (virt >> 12) & 0x1FF;

    if (!(m_pml4->entries[pml4_idx] & (uint64_t)PageFlags::Present)) {
        fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PML4");
        return 0;
    }

    PageTable* pdpt =
        reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000);

    if (!(pdpt->entries[pdpt_idx] & (uint64_t)PageFlags::Present)) {
        fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PDPT");
        return 0;
    }

    PageTable* pd =
        reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000);

    if (!(pd->entries[pd_idx] & (uint64_t)PageFlags::Present)) {
        fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PD");
        return 0;
    }

    PageTable* pt =
        reinterpret_cast<PageTable*>(pd->entries[pd_idx] & 0x000FFFFFFFFFF000);

    if (!(pt->entries[pt_idx] & (uint64_t)PageFlags::Present)) {
        fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PT");
        return 0;
    }

    uintptr_t phys =
        (pt->entries[pt_idx] & 0x000FFFFFFFFFF000) + (virt & 0xFFF);
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "VIRTUAL MEMORY MANAGER",
        "Translate: virt=%p -> phys=%p",
        virt,
        phys
    );
*/
    return phys;
}
