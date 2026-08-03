#pragma once

#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/types.h>

struct Task;

namespace fkernel {
namespace ipc {

// Contiguous physical allocation for DMA; mapped cache-disabled.
class DmaShm {
  uintptr_t m_phys_base{0};
  size_t    m_order{0};      // buddy order (page count = 1 << (order - 12))
  size_t    m_size_bytes{0};
  uint8_t   m_ref_count{1};

  DmaShm(uintptr_t phys, size_t order, size_t bytes);
  ~DmaShm();

public:
  DmaShm(const DmaShm&) = delete;
  DmaShm& operator=(const DmaShm&) = delete;

  static DmaShm* create(size_t bytes);

  void ref()   { __sync_fetch_and_add(&m_ref_count, 1u); }
  void unref() { if (__sync_fetch_and_sub(&m_ref_count, 1u) == 1u) delete this; }

  void map_into(Task* task, uintptr_t vaddr);
  void unmap_from(Task* task, uintptr_t vaddr);

  uintptr_t phys_base()   const { return m_phys_base; }
  size_t    size_bytes()  const { return m_size_bytes; }
};

}
}
