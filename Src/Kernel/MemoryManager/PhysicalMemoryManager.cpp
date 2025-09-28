#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <LibC/limits.h>
#include <LibFK/log.h>
#include <LibFK/new.h>  
#include <LibFK/rb_tree.h>

void PhysicalMemoryManager::initialize(const multiboot2::TagMemoryMap* mmap) {
    if (is_initialized){
        kerror("PHYSICAL MEMORY MANAGER", "Physical Memory Manager is already initialized!");
        return;
    }

    for (auto& entry : *mmap) {
        uintptr_t entry_start = static_cast<uintptr_t>(entry.base_addr);
        uintptr_t entry_end = entry_start + static_cast<uintptr_t>(entry.length);
        MemoryType type = static_cast<MemoryType>(entry.type);

        PhysicalMemoryRange range {
            .m_start = entry_start,
            .m_end = entry_end,
            .m_type = type,
            .m_bitmap = Bitmap<uint64_t, MAX_CHUNKS_PER_RANGE>()
        };

        m_memory_ranges.insert(range);
        klog("PHYSICAL MEMORY MANAGER", "Insert memory range: [0x%lx - 0x%lx] type: %u", entry_start, entry_end, entry.type);
    }


    klog("PHYSICAL MEMORY MANAGER", "Initialized Physical Memory Manager...");
}