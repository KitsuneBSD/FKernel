#pragma once

#include <Kernel/Memory/VirtualMemory/Pages/PageFlags.h>
#include <Kernel/Memory/VirtualMemory/Pages/PageTable.h>


extern "C" void write_on_cr3(void *pml4_virt_addr);
extern "C" uintptr_t read_on_cr3();

class VirtualMemoryManager {
private:
    PageTable* m_pml4 = nullptr;
    uintptr_t  m_pml4_phys = 0;

protected:

    PageTable* alloc_page_table();
    void perform_initial_identity_mapping();
    void invlpg(uintptr_t addr);

    uintptr_t get_table_virtual_address(uint16_t pml4_idx, uint16_t pdpt_idx = 0,
                                        uint16_t pd_idx = 0, uint16_t pt_idx = 0) const;

public:
    VirtualMemoryManager();
    VirtualMemoryManager(const VirtualMemoryManager&) = delete;
    VirtualMemoryManager& operator=(const VirtualMemoryManager&) = delete;

    /**
     * @brief Returns the singleton instance of the VirtualMemoryManager.
     */
    static VirtualMemoryManager& the();

    /**
     * @brief Initializes the virtual memory manager.
     *
     * Allocates PML4, maps it and other kernel structures to dedicated virtual
     * addresses, and sets up initial identity mapping for the lower memory region.
     */
    void initialize();
    
    void map_page(uintptr_t virt, uintptr_t phys, PageFlags flags);
    void map_range(uintptr_t start, uintptr_t size, PageFlags flags);
    void unmap_page(uintptr_t virt);
    uintptr_t translate(uintptr_t virt); // Translate virtual address to physical
};

