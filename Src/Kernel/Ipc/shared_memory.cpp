#include <Kernel/Ipc/shared_memory.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Task/task.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {
namespace ipc {

SharedMemory::SharedMemory(size_t pages)
    : m_size_pages(pages) {
  m_phys_pages = new uintptr_t[pages];
  for (size_t i = 0; i < pages; ++i) {
    m_phys_pages[i] = PhysicalMemoryManager::the().alloc_page();
    if (m_phys_pages[i] == 0) {
      fk::algorithms::kwarn("SHARED_MEMORY", "alloc_page failed at index %zu", i);
      for (size_t j = 0; j < i; ++j)
        PhysicalMemoryManager::the().free_page(m_phys_pages[j]);
      delete[] m_phys_pages;
      m_phys_pages = nullptr;
      m_size_pages = 0;
      return;
    }
  }
}

SharedMemory::~SharedMemory() {
  if (m_phys_pages) {
    for (size_t i = 0; i < m_size_pages; ++i) {
      if (m_phys_pages[i])
        PhysicalMemoryManager::the().free_page(m_phys_pages[i]);
    }
    delete[] m_phys_pages;
    m_phys_pages = nullptr;
  }
}

SharedMemory* SharedMemory::create(size_t bytes) {
  if (bytes == 0) return nullptr;
  size_t pages = (bytes + 4095) / 4096;
  auto* shm = new SharedMemory(pages);
  if (shm->m_phys_pages == nullptr) {
    delete shm;
    return nullptr;
  }
  return shm;
}

void SharedMemory::map_into(Task* task, uintptr_t vaddr, PageFlags prot) {
  if (!m_phys_pages || !task) return;

  uintptr_t curr_vaddr = vaddr;
  for (size_t i = 0; i < m_size_pages; ++i) {
    if (m_phys_pages[i] == 0) continue;
    VirtualMemoryManager::the().map_page(curr_vaddr, m_phys_pages[i], prot);
    curr_vaddr += 4096;
  }
}

void SharedMemory::unmap_from(Task* task, uintptr_t vaddr) {
  if (!task) return;

  uintptr_t curr_vaddr = vaddr;
  for (size_t i = 0; i < m_size_pages; ++i) {
    VirtualMemoryManager::the().unmap_page(curr_vaddr);
    curr_vaddr += 4096;
  }
}

}
}
