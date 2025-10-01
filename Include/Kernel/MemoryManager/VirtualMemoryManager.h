#pragma once 

#include <LibC/stdint.h>
#include <LibC/stddef.h>

#include <Kernel/MemoryManager/Pages/PageFlags.h>

class VirtualMemoryManager{
private:
VirtualMemoryManager() = default;
VirtualMemoryManager(const VirtualMemoryManager&) = delete;
VirtualMemoryManager& operator=(const VirtualMemoryManager&) = delete;

private:
    bool m_is_initialized = false;
    uint64_t* m_pml4 = nullptr;
    uint64_t* alloc_table();

public:
    static VirtualMemoryManager& the() {
        static VirtualMemoryManager inst;
        return inst; 
    }

    void initialize();
    void map_page(uintptr_t virt, uintptr_t phys, uint64_t flags);
    void map_range(uintptr_t virt_start, uintptr_t phys_start, size_t size, uint64_t flags);
    uintptr_t virt_to_phys(uintptr_t virt);
};