#pragma once

#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Memory/ref_ptr.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>

namespace fkernel {

enum class IoCompletionStatus : uint8_t { Success, Error, Timeout, Busy };

// Abstract base class for interrupt-driven I/O operations
class AsyncIoOperation {
public:
  virtual ~AsyncIoOperation() = default;

  // Called when interrupt occurs
  virtual void on_interrupt(uint32_t interrupt_status) = 0;

  // Check if operation completed
  virtual bool is_completed() const = 0;

  // Get completion status
  virtual IoCompletionStatus get_status() const = 0;

  // Block caller until operation completes
  virtual IoCompletionStatus wait_for_completion(uint64_t timeout_ms = 5000) = 0;
};

// Interrupt-driven I/O request queue
template <typename T, size_t MAX_PENDING = 32> class AsyncIoQueue {
private:
  T m_pending_operations[MAX_PENDING];
  bool m_active[MAX_PENDING] = {false};
  uint32_t m_head = 0;
  uint32_t m_tail = 0;
  uint32_t m_count = 0;

public:
  bool enqueue(T operation) {
    if (m_count >= MAX_PENDING) {
      return false;
    }

    m_pending_operations[m_tail] = operation;
    m_active[m_tail] = true;
    m_tail = (m_tail + 1) % MAX_PENDING;
    m_count++;
    return true;
  }

  T dequeue() {
    if (m_count == 0) {
      return nullptr;
    }

    T operation = m_pending_operations[m_head];
    m_active[m_head] = false;
    m_head = (m_head + 1) % MAX_PENDING;
    m_count--;
    return operation;
  }

  T* find_pending(uint32_t slot) {
    for (uint32_t i = 0; i < MAX_PENDING; ++i) {
      if (m_active[i] && m_pending_operations[i] && m_pending_operations[i]->get_slot() == slot) {
        return &m_pending_operations[i];
      }
    }
    return nullptr;
  }

  bool is_full() const { return m_count >= MAX_PENDING; }
  bool is_empty() const { return m_count == 0; }
  uint32_t size() const { return m_count; }
};

// Interrupt-driven DMA buffer manager
class DmaBuffer {
private:
  void* m_virtual_addr = nullptr;
  uintptr_t m_physical_addr = 0;
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
    // Simplificação: ordem do buddy baseada no número de páginas
    // Em um kernel real, converteríamos page_count para a potência de 2 mais próxima (order)
    m_physical_addr = MemoryManager::the().allocate_contiguous(page_count);
    if (m_physical_addr == 0) {
      return fk::core::Error::OutOfMemory;
    }

    // Map to virtual address
    for (size_t offset = 0; offset < size; offset += 4096) {
      uintptr_t phys = m_physical_addr + offset;
      uintptr_t virt = phys; // Identity map para drivers kernel-space simples

      MemoryManager::the().map_page(
          virt, phys,
          static_cast<PageFlags>(PageFlags::Present | PageFlags::Writable |
                                 PageFlags::CacheDisabled));
    }

    m_virtual_addr = reinterpret_cast<void*>(m_physical_addr);
    m_size = size;
    m_mapped = true;

    // Clear buffer
    memset(m_virtual_addr, 0, m_size);

    return {};
  }

  void cleanup() {
    if (!m_mapped)
      return;

    // Unmap pages
    for (size_t offset = 0; offset < m_size; offset += 4096) {
      uintptr_t virt = reinterpret_cast<uintptr_t>(m_virtual_addr) + offset;
      MemoryManager::the().unmap_page(virt);
    }

    // Free physical memory
    size_t page_count = (m_size + 4095) / 4096;
    MemoryManager::the().free_contiguous(m_physical_addr, page_count);

    m_virtual_addr = nullptr;
    m_physical_addr = 0;
    m_size = 0;
    m_mapped = false;
  }

  void* virtual_address() const { return m_virtual_addr; }
  uintptr_t physical_address() const { return m_physical_addr; }
  size_t size() const { return m_size; }
  bool is_valid() const { return m_mapped; }
};

} // namespace fkernel
