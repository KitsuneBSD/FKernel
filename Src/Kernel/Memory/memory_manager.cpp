#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/iommu.h>
#include <Kernel/Arch/x86_64/Memory/IntelIOMMU/vtd.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/allocator_backend.h>
#include <LibFK/Utilities/memory.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

#include <Kernel/Clock/clock_interrupt.h>

extern "C" uint8_t __heap_start[];
extern "C" uint8_t __heap_end[];

void MemoryManager::initialize() {
  assert(!m_is_initialized && "MemoryManager: Double initialization attempted!");
  assert(boot::BootInfo::the().is_initialized() && "MemoryManager: BootInfo not initialized!");

  PhysicalMemoryManager::the().initialize();
  VirtualMemoryManager::the().initialize();

  // Initialize Intel IOMMU (VT-d)
  fkernel::vtd::IntelIOMMU::the().initialize();

  initialize_heap();

  HardwareInterruptManager::the().set_memory_manager(true);
  TimerManager::the().set_memory_manager(true);
  ClockManager::the().set_memory_manager(true);

  m_is_initialized = true;
  fk::algorithms::klog("MEMORY", "MemoryManager initialized");
}

fkernel::IOMMU* MemoryManager::get_iommu() const {
    if (fkernel::vtd::IntelIOMMU::the().is_active()) {
        return static_cast<fkernel::IOMMU*>(&fkernel::vtd::IntelIOMMU::the());
    }
    return nullptr;
}

void MemoryManager::initialize_heap() {
    if (m_heap_initialized) return;

    static fk::memory::AllocatorBackend backend = {
        .allocate = [](size_t size) -> void* { return the().allocate(size); },
        .reallocate = [](void* ptr, size_t size) -> void* { return the().reallocate(ptr, size); },
        .free = [](void* ptr) { the().free(ptr); }
    };
    fk::memory::set_allocator_backend(&backend);

    uintptr_t start_addr = (reinterpret_cast<uintptr_t>(__heap_start) + 15) & ~15;
    uintptr_t end_addr = reinterpret_cast<uintptr_t>(__heap_end) & ~15;
    size_t total_size = end_addr - start_addr;

    ASSERT(total_size > sizeof(BlockHeader));

    m_heap_head = reinterpret_cast<BlockHeader*>(start_addr);
    m_heap_head->size = total_size - sizeof(BlockHeader);
    m_heap_head->is_free = true;
    m_heap_head->next = nullptr;
    m_heap_head->prev = nullptr;
    m_heap_head->magic = BlockHeader::MAGIC;

    m_heap_initialized = true;
    libc_set_heap_ready();
    fk::algorithms::klog("MEMORY", "Kernel Heap initialized. Size: %zu bytes", total_size);
}

static inline uint64_t save_and_disable_interrupts() {
    return arch_save_flags_and_disable();
}

static inline void restore_interrupts(uint64_t flags) {
    arch_restore_flags(flags);
}

void* MemoryManager::allocate(size_t size) {
    if (!m_heap_initialized) return nullptr;

    uint64_t flags = save_and_disable_interrupts();
    m_heap_lock.lock();

    // Align size to 16 bytes
    size = (size + 15) & ~15;

    BlockHeader* current = m_heap_head;
    while (current) {
        if (current->magic != BlockHeader::MAGIC) {
            fk::algorithms::kfatal("MEMORY", "Kernel Heap Corruption at %p! Magic: 0x%lx Expected: 0x%lx", 
                                  current, (uint64_t)current->magic, (uint64_t)BlockHeader::MAGIC);
        }

        if (current->is_free && current->size >= size) {
            if (current->size >= size + sizeof(BlockHeader) + 16) {
                BlockHeader* new_block = reinterpret_cast<BlockHeader*>(
                    reinterpret_cast<uint8_t*>(current) + sizeof(BlockHeader) + size
                );
                
                new_block->size = current->size - size - sizeof(BlockHeader);
                new_block->is_free = true;
                new_block->next = current->next;
                new_block->prev = current;
                new_block->magic = BlockHeader::MAGIC;
                if (new_block->next)
                    new_block->next->prev = new_block;

                current->size = size;
                current->next = new_block;
            }

            current->is_free = false;
            void* ptr = reinterpret_cast<void*>(reinterpret_cast<uint8_t*>(current) + sizeof(BlockHeader));
            m_heap_lock.unlock();
            restore_interrupts(flags);
            return ptr;
        }
        current = current->next;
    }

    m_heap_lock.unlock();
    restore_interrupts(flags);
    fk::algorithms::kerror("MEMORY", "Kernel Heap: Out of memory! Requested: %zu", size);
    return nullptr;
}

void* MemoryManager::reallocate(void* ptr, size_t size) {
    if (!ptr) return allocate(size);
    if (size == 0) {
        free(ptr);
        return nullptr;
    }

    uint64_t flags = save_and_disable_interrupts();
    m_heap_lock.lock();

    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader)
    );

    if (header->magic != BlockHeader::MAGIC) {
        fk::algorithms::kfatal("MEMORY", "Kernel Heap Corruption (realloc) at %p! Magic: 0x%lx", ptr, (uint64_t)header->magic);
    }

    size_t old_size = header->size;
    if (size <= old_size) {
        m_heap_lock.unlock();
        restore_interrupts(flags);
        return ptr;
    }
    m_heap_lock.unlock();
    restore_interrupts(flags);

    void* new_ptr = allocate(size);
    if (!new_ptr) return nullptr;

    fk::memory::copy(new_ptr, ptr, old_size);
    free(ptr);
    
    return new_ptr;
}

void MemoryManager::free(void* ptr) {
    if (!ptr) return;

    uint64_t flags = save_and_disable_interrupts();
    m_heap_lock.lock();

    BlockHeader* header = reinterpret_cast<BlockHeader*>(
        reinterpret_cast<uint8_t*>(ptr) - sizeof(BlockHeader)
    );

    if (header->magic != BlockHeader::MAGIC) {
        fk::algorithms::kfatal("MEMORY", "Kernel Heap Corruption (free) at %p! Magic: 0x%lx", ptr, (uint64_t)header->magic);
    }

    if (header->is_free) {
        fk::algorithms::kwarn("MEMORY", "Kernel Heap: Block at %p is already free!", ptr);
        m_heap_lock.unlock();
        restore_interrupts(flags);
        return;
    }

    header->is_free = true;

    // Merge with next free block
    if (header->next && header->next->is_free && header->next->magic == BlockHeader::MAGIC) {
        BlockHeader* next = header->next;
        header->size += sizeof(BlockHeader) + next->size;
        header->next = next->next;
        if (header->next)
            header->next->prev = header;
    }

    // Merge with previous free block
    if (header->prev && header->prev->is_free && header->prev->magic == BlockHeader::MAGIC) {
        BlockHeader* prev = header->prev;
        prev->size += sizeof(BlockHeader) + header->size;
        prev->next = header->next;
        if (prev->next)
            prev->next->prev = prev;
    }

    m_heap_lock.unlock();
    restore_interrupts(flags);
}

void MemoryManager::map_page(uintptr_t virt, uintptr_t phys, PageFlags flags){
  VirtualMemoryManager::the().map_page(virt, phys, flags);
}

void MemoryManager::unmap_page(uintptr_t virt) {
    VirtualMemoryManager::the().unmap_page(virt);
}

uintptr_t MemoryManager::translate(uintptr_t virt) {
    return VirtualMemoryManager::the().translate(virt);
}

uintptr_t MemoryManager::allocate_page(ZoneType preferred, uint32_t preferred_node) {
    return PhysicalMemoryManager::the().alloc_page(preferred, preferred_node);
}

void MemoryManager::free_page(uintptr_t phys) {
    PhysicalMemoryManager::the().free_page(phys);
}

uintptr_t MemoryManager::allocate_contiguous(size_t order, ZoneType preferred, uint32_t preferred_node) {
    return PhysicalMemoryManager::the().alloc_contiguous(order, preferred, preferred_node);
}

void MemoryManager::free_contiguous(uintptr_t phys, size_t order) {
    PhysicalMemoryManager::the().free_contiguous(phys, order);
}

void MemoryManager::heap_stats(size_t& total_out, size_t& free_out) const {
    total_out = 0;
    free_out = 0;
    if (!m_heap_head) return;
    BlockHeader* curr = m_heap_head;
    while (curr) {
        total_out += curr->size;
        if (curr->is_free) free_out += curr->size;
        curr = curr->next;
    }
}
