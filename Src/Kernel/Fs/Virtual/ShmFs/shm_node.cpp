#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Fs/Virtual/ShmFs/shm_node.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Scheduler/Task/task.h>

namespace fkernel {

fk::core::Result<fk::RefPtr<ShmNode>, fk::core::Error>
ShmNode::create() {
  return fk::RefPtr<ShmNode>(new ShmNode());
}

ShmNode::ShmNode() {}

ShmNode::~ShmNode() {
  if (m_shm) {
    m_shm->unref();
    m_shm = nullptr;
  }
}

size_t ShmNode::size() const {
  return m_logical_size;
}

fk::core::Result<size_t, fk::core::Error>
ShmNode::read([[maybe_unused]] uint64_t offset, size_t size, uint8_t* buf) {
  if (!m_shm || offset >= m_logical_size) return 0;
  size_t avail = m_logical_size - offset;
  size_t n = (size < avail) ? size : avail;

  uintptr_t vaddr = offset;
  uintptr_t phys = VirtualMemoryManager::the().translate(vaddr);
  fk::memory::copy(buf, reinterpret_cast<void*>(phys), n);
  return n;
}

fk::core::Result<size_t, fk::core::Error>
ShmNode::write([[maybe_unused]] uint64_t offset, size_t size, const uint8_t* buf) {
  if (!m_shm || offset >= m_logical_size) return fk::core::Error::InvalidParameter;
  size_t avail = m_logical_size - offset;
  size_t n = (size < avail) ? size : avail;

  uintptr_t vaddr = offset;
  uintptr_t phys = VirtualMemoryManager::the().translate(vaddr);
  fk::memory::copy(reinterpret_cast<void*>(phys), buf, n);
  return n;
}

fk::core::Result<void, fk::core::Error>
ShmNode::truncate(uint64_t new_size) {
  m_lock.lock();

  if (m_shm && m_shm->size_bytes() >= new_size) {
    m_logical_size = new_size;
    m_lock.unlock();
    return {};
  }

  if (m_shm) {
    m_shm->unref();
    m_shm = nullptr;
  }

  if (new_size > 0) {
    m_shm = ipc::SharedMemory::create(new_size);
    if (!m_shm) {
      m_lock.unlock();
      return fk::core::Error::OutOfMemory;
    }
    m_shm->ref();
  }

  m_logical_size = new_size;
  m_lock.unlock();
  return {};
}

void ShmNode::map_into(Task* task, uintptr_t vaddr, PageFlags prot) {
  if (m_shm) m_shm->map_into(task, vaddr, prot);
}

void ShmNode::unmap_from(Task* task, uintptr_t vaddr) {
  if (m_shm) m_shm->unmap_from(task, vaddr);
}

}
