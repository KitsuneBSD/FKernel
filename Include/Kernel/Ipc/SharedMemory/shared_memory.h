#pragma once

#include <Kernel/Ipc/SharedMemory/shared_memory_mapping.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Types/types.h>

struct Task;

namespace fkernel {
namespace ipc {

class SharedMemory {
  size_t m_size_pages{0};
  uintptr_t* m_phys_pages{nullptr};
  uint64_t m_generation{0};
  uint8_t m_ref_count{1};

  SharedMemory(size_t pages);
  ~SharedMemory();

public:
  SharedMemory(const SharedMemory&) = delete;
  SharedMemory& operator=(const SharedMemory&) = delete;

  static SharedMemory* create(size_t bytes);

  void ref() { __sync_fetch_and_add(&m_ref_count, 1u); }
  void unref() { if (__sync_fetch_and_sub(&m_ref_count, 1u) == 1u) delete this; }

  void map_into(Task* task, uintptr_t vaddr, PageFlags prot);
  void unmap_from(Task* task, uintptr_t vaddr);
  void revoke() { ++m_generation; }

  const uint64_t* generation_ptr() const { return &m_generation; }
  uint64_t generation() const { return m_generation; }
  size_t size_bytes() const { return m_size_pages * 4096; }
  size_t page_count() const { return m_size_pages; }
};

}
}
