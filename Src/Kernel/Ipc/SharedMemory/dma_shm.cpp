#include <Kernel/Ipc/SharedMemory/dma_shm.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <LibFK/Algorithms/Logging/log.h>

namespace fkernel {
namespace ipc {

DmaShm::DmaShm(uintptr_t phys, size_t order, size_t bytes)
    : m_phys_base(phys), m_order(order), m_size_bytes(bytes) {}

DmaShm::~DmaShm() {
    if (m_phys_base)
        PhysicalMemoryManager::the().free_contiguous(m_phys_base, m_order);
}

DmaShm* DmaShm::create(size_t bytes) {
    if (bytes == 0) return nullptr;
    size_t order = size_to_order(bytes);
    uintptr_t phys = PhysicalMemoryManager::the().alloc_contiguous(order);
    if (phys == 0) {
        fk::algorithms::kwarn("DMA_SHM", "alloc_contiguous order=%zu failed", order);
        return nullptr;
    }
    auto* obj = new DmaShm(phys, order, size_t(1) << order);
    if (!obj) {
        PhysicalMemoryManager::the().free_contiguous(phys, order);
        return nullptr;
    }
    return obj;
}

void DmaShm::map_into(Task* task, uintptr_t vaddr) {
    if (!task || !m_phys_base) return;
    size_t page_count = (m_size_bytes + 4095) / 4096;
    PageFlags flags = PageFlags::Writable | PageFlags::User | PageFlags::CacheDisabled;
    for (size_t i = 0; i < page_count; ++i)
        VirtualMemoryManager::the().map_page(vaddr + i * 4096, m_phys_base + i * 4096, flags);
}

void DmaShm::unmap_from(Task* task, uintptr_t vaddr) {
    if (!task || !m_phys_base) return;
    size_t page_count = (m_size_bytes + 4095) / 4096;
    for (size_t i = 0; i < page_count; ++i)
        VirtualMemoryManager::the().unmap_page(vaddr + i * 4096);
}

}
}
