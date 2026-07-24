#pragma once

#include <Kernel/Driver/Storage/Nvme/nvme_register_access.h>
#include <LibFK/Types/types.h>
#include <LibFK/Core/result.h>

namespace fkernel {

class NvmeQueueManager {
public:
  struct NvmeCompletion {
    uint32_t status : 32;
    uint16_t command_id : 16;
    uint16_t rsvd : 16;
  };

  struct Queue {
    void* memory = nullptr;
    uint32_t size = 0;
    uint16_t head = 0;
    uint16_t tail = 0;
    bool phase = true;
    uint32_t phys_addr = 0;
    volatile uint32_t* head_db = nullptr;
    void* cq_memory = nullptr;
    uint32_t cq_size = 0;
    uint16_t cq_head = 0;
    bool cq_phase = true;
    uint32_t cq_phys_addr = 0;
    volatile uint32_t* cq_head_db = nullptr;
  };

  NvmeQueueManager(NvmeRegisterAccess& registers);

  // Additional methods
  fk::core::Result<void, fk::core::Error> setup_admin_queue();
  fk::core::Result<void, fk::core::Error> setup_io_queue(uint32_t queue_id);
  fk::core::Result<void, fk::core::Error> reset_controller();
  fk::core::Result<void, fk::core::Error> enable_interrupts();

  // Accessors
  const Queue& admin_queue() const { return m_admin_queue; }
  const Queue& io_queue(uint32_t id) const { return m_io_queues[id]; }

  // Command submission and completion processing
  fk::core::Result<void, fk::core::Error> submit_command(const void* command, uint32_t queue_id);
  fk::core::Result<void, fk::core::Error> process_completions();

private:
  fk::core::Result<uintptr_t, fk::core::Error> allocate_dma_memory(size_t size);
  void free_dma_memory(uintptr_t phys_addr, size_t size);

  NvmeRegisterAccess& m_registers;
  Queue m_admin_queue;
  Queue m_io_queues[8];
  bool m_interrupts_enabled = false;
};

} // namespace fkernel