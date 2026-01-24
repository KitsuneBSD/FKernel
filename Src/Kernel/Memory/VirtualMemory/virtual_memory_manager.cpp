#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

VirtualMemoryManager::VirtualMemoryManager()
    : m_pml4(nullptr), m_pml4_phys(0) {
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Ctor (empty)");
    */
}

VirtualMemoryManager& VirtualMemoryManager::the() {
    static VirtualMemoryManager inst;
    return inst;
}

void VirtualMemoryManager::invlpg(uintptr_t addr) {
    asm volatile("invlpg (%0)" :: "r"(addr) : "memory");
}

void VirtualMemoryManager::flush_tlb() {
    uintptr_t cr3;
    asm volatile("mov %%cr3, %0" : "=r"(cr3));
    asm volatile("mov %0, %%cr3" :: "r"(cr3) : "memory");
}

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

    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "VIRTUAL MEMORY MANAGER",
        "PML4 allocated: phys=%p",
        m_pml4_phys
    );
    */

    m_pml4 = reinterpret_cast<PageTable*>(m_pml4_phys);
    memset(m_pml4, 0, PAGE_SIZE);

    perform_initial_identity_mapping();
    if (boot::BootInfo::the().has_framebuffer()) {
        auto fb = boot::BootInfo::the().get_framebuffer_info();
        uintptr_t start = fb.addr & ~0xFFFULL;
        uintptr_t end = (fb.addr + fb.pitch * fb.height + 0xFFF) & ~0xFFFULL;
        for (uintptr_t v = start; v < end; v += 0x1000) {
            map_page(v, v, PageFlags::Present | PageFlags::Writable);
        }
        fk::algorithms::klog("VIRTUAL MEMORY MANAGER", "Mapped framebuffer: %p - %p", (void*)start, (void*)end);
    }

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

    uint64_t user_bit = static_cast<uint64_t>(flags) & static_cast<uint64_t>(PageFlags::User);
    uint64_t write_bit = static_cast<uint64_t>(flags) & static_cast<uint64_t>(PageFlags::Writable);
    bool changed_parents = false;

    // PML4
    if (!(m_pml4->entries[pml4_idx] & static_cast<uint64_t>(PageFlags::Present))) {
        uintptr_t new_pdpt = PhysicalMemoryManager::the().alloc_page();
        assert(new_pdpt != 0);
        memset(reinterpret_cast<void*>(new_pdpt), 0, PAGE_SIZE);
        m_pml4->entries[pml4_idx] = new_pdpt | static_cast<uint64_t>(PageFlags::Present) | write_bit | user_bit;
        changed_parents = true;
    } else {
        uint64_t original = m_pml4->entries[pml4_idx];
        m_pml4->entries[pml4_idx] |= (user_bit | write_bit);
        if (m_pml4->entries[pml4_idx] != original) changed_parents = true;
    }
    PageTable* pdpt = reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000);

    // PDPT
    if (!(pdpt->entries[pdpt_idx] & static_cast<uint64_t>(PageFlags::Present))) {
        uintptr_t new_pd = PhysicalMemoryManager::the().alloc_page();
        assert(new_pd != 0);
        memset(reinterpret_cast<void*>(new_pd), 0, PAGE_SIZE);
        pdpt->entries[pdpt_idx] = new_pd | static_cast<uint64_t>(PageFlags::Present) | write_bit | user_bit;
        changed_parents = true;
    } else {
        uint64_t original = pdpt->entries[pdpt_idx];
        pdpt->entries[pdpt_idx] |= (user_bit | write_bit);
        if (pdpt->entries[pdpt_idx] != original) changed_parents = true;
    }
    PageTable* pd = reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000);

    // PD
    if (!(pd->entries[pd_idx] & static_cast<uint64_t>(PageFlags::Present))) {
        uintptr_t new_pt = PhysicalMemoryManager::the().alloc_page();
        assert(new_pt != 0);
        memset(reinterpret_cast<void*>(new_pt), 0, PAGE_SIZE);
        pd->entries[pd_idx] = new_pt | static_cast<uint64_t>(PageFlags::Present) | write_bit | user_bit;
        changed_parents = true;
    } else {
        uint64_t original = pd->entries[pd_idx];
        pd->entries[pd_idx] |= (user_bit | write_bit);
        if (pd->entries[pd_idx] != original) changed_parents = true;
    }
    PageTable* pt = reinterpret_cast<PageTable*>(pd->entries[pd_idx] & 0x000FFFFFFFFFF000);

    // PT
    pt->entries[pt_idx] = phys | static_cast<uint64_t>(flags) | static_cast<uint64_t>(PageFlags::Present);
    
    if (changed_parents) {
        flush_tlb();
    } else {
        invlpg(virt);
    }

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
        // fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PML4");
        return 0;
    }

    PageTable* pdpt =
        reinterpret_cast<PageTable*>(m_pml4->entries[pml4_idx] & 0x000FFFFFFFFFF000);

    if (!(pdpt->entries[pdpt_idx] & (uint64_t)PageFlags::Present)) {
        // fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PDPT");
        return 0;
    }

    PageTable* pd =
        reinterpret_cast<PageTable*>(pdpt->entries[pdpt_idx] & 0x000FFFFFFFFFF000);

    if (!(pd->entries[pd_idx] & (uint64_t)PageFlags::Present)) {
        // fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PD");
        return 0;
    }

    PageTable* pt =
        reinterpret_cast<PageTable*>(pd->entries[pd_idx] & 0x000FFFFFFFFFF000);

    if (!(pt->entries[pt_idx] & (uint64_t)PageFlags::Present)) {
        // fk::algorithms::kwarn("VIRTUAL MEMORY MANAGER", "Translate fail: no PT");
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

uintptr_t clone_table_recursive(uintptr_t old_phys, int level, bool deep_copy) {
    uintptr_t new_phys = PhysicalMemoryManager::the().alloc_page();
    PageTable *old_table = reinterpret_cast<PageTable *>(old_phys);
    PageTable *new_table = reinterpret_cast<PageTable *>(new_phys);
    memset(new_table, 0, 0x1000);

    for (int i = 0; i < 512; ++i) {
        if (!(old_table->entries[i] & 1)) continue; // Not present

        // Kernel-only mappings (no User bit) are shared by copying the entry
        if (!(old_table->entries[i] & 4)) {
            new_table->entries[i] = old_table->entries[i];
            continue;
        }

        // User mappings:
        if (level > 1) {
            // Recurse into sub-tables
            uintptr_t old_sub = old_table->entries[i] & 0x000FFFFFFFFFF000;
            uintptr_t new_sub = clone_table_recursive(old_sub, level - 1, deep_copy);
            new_table->entries[i] = new_sub | (old_table->entries[i] & 0xFFF);
        } else {
            // It's a PT, pointing to a page
            if (deep_copy) {
                uintptr_t old_page = old_table->entries[i] & 0x000FFFFFFFFFF000;
                uintptr_t new_page = PhysicalMemoryManager::the().alloc_page();
                memcpy(reinterpret_cast<void *>(new_page), reinterpret_cast<void *>(old_page), 0x1000);
                new_table->entries[i] = new_page | (old_table->entries[i] & 0xFFF);
            } else {
                // For create_address_space (execve), we don't copy user pages
                new_table->entries[i] = 0;
            }
        }
    }
    return new_phys;
}

uintptr_t VirtualMemoryManager::create_address_space() {
    // Create a new address space by cloning the current one but WITHOUT user pages.
    // This keeps the kernel mappings.
    return clone_table_recursive(m_pml4_phys, 4, false);
}

uintptr_t VirtualMemoryManager::clone_address_space(uintptr_t source_cr3) {
    // Deep copy for fork()
    return clone_table_recursive(source_cr3, 4, true);
}

void VirtualMemoryManager::switch_address_space(uintptr_t cr3) {
    if (cr3 == 0) return;
    m_pml4_phys = cr3;
    m_pml4 = reinterpret_cast<PageTable *>(cr3);
    write_on_cr3(reinterpret_cast<void *>(cr3));
}
