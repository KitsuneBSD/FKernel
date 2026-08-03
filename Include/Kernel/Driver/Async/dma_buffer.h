#pragma once

#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/Memory/physical_address.h>
#include <LibFK/Core/result.h>
#include <LibFK/Core/error.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <LibFK/Types/types.h>
#include <LibFK/Utilities/memory.h>

namespace fkernel {

class DmaBuffer {
private:
  void* m_virtual_addr = nullptr;
  fk::PhysicalAddress m_physical_addr;
  size_t m_size = 0;
  bool m_mapped = false;

public:
  DmaBuffer() = default;
  ~DmaBuffer() { cleanup(); }

  fk::core::Result<void, fk::core::Error> allocate(size_t size) {
    if (m_mapped) {
      cleanup();
    }

    size_t page_count = (size + 4095) / 4096;
    m_physical_addr = fk::PhysicalAddress(MemoryManager::the().allocate_contiguous(page_count));
    if (m_physical_addr.is_null()) {
      return fk::core::Error::OutOfMemory;
    }

    for (size_t offset = 0; offset < size; offset += 4096) {
      uintptr_t phys = m_physical_addr.as_uintptr() + offset;
      uintptr_t virt = phys;

      MemoryManager::the().map_page(
          virt, phys,
          static_cast<PageFlags>(PageFlags::Present | PageFlags::Writable |
                                 PageFlags::CacheDisabled));
    }

    m_virtual_addr = reinterpret_cast<void*>(m_physical_addr.as_uintptr());
    m_size = size;
    m_mapped = true;

    memset(m_virtual_addr, 0, m_size);

    return {};
  }

  void cleanup() {
    if (!m_mapped)
      return;

    for (size_t offset = 0; offset < m_size; offset += 4096) {
      uintptr_t virt = reinterpret_cast<uintptr_t>(m_virtual_addr) + offset;
      MemoryManager::the().unmap_page(virt);
    }

    size_t page_count = (m_size + 4095) / 4096;
    MemoryManager::the().free_contiguous(m_physical_addr.as_uintptr(), page_count);

    m_virtual_addr = nullptr;
    m_physical_addr = fk::PhysicalAddress(0);
    m_size = 0;
    m_mapped = false;
  }

  void* virtual_address() const { return m_virtual_addr; }
  fk::PhysicalAddress physical_address() const { return m_physical_addr; }
  size_t size() const { return m_size; }
  bool is_valid() const { return m_mapped; }
};

} // namespace fkernel
