#pragma once

#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Ipc/SharedMemory/shared_memory.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <LibFK/Core/result.h>
#include <LibFK/Synchronization/spinlock.h>
#include <LibFK/Types/types.h>

namespace fkernel {

class ShmNode final : public Node {
public:
  static fk::core::Result<fk::RefPtr<ShmNode>, fk::core::Error> create();

  ShmNode();
  ~ShmNode() override;

  fk::core::Result<size_t, fk::core::Error> read(uint64_t offset, size_t size, uint8_t* buf) override;
  fk::core::Result<size_t, fk::core::Error> write(uint64_t offset, size_t size, const uint8_t* buf) override;
  size_t size() const override;
  bool is_directory() const override { return false; }
  bool is_shm() const override { return true; }

  fk::core::Result<void, fk::core::Error> truncate(uint64_t new_size) override;

  ipc::SharedMemory* shm() const { return m_shm; }
  void map_into(Task* task, uintptr_t vaddr, PageFlags prot);
  void unmap_from(Task* task, uintptr_t vaddr);

private:
  mutable fk::synchronization::Spinlock m_lock;
  ipc::SharedMemory* m_shm{nullptr};
  size_t m_logical_size{0};
};

}
