#pragma once

#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_queue_descriptor.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_register_access.h>
#include <LibFK/Types/types.h>
#include <LibFK/Core/result.h>

namespace fkernel {

class NvmeQueueManager {
public:
  NvmeQueueManager(NvmeRegisterAccess& registers);

  fk::core::Result<void, fk::core::Error> setup_admin_queue();
  fk::core::Result<void, fk::core::Error> setup_io_queue(uint32_t queue_id);
  fk::core::Result<void, fk::core::Error> reset_controller();
  fk::core::Result<void, fk::core::Error> enable_interrupts();

  const NvmeQueueDescriptor& admin_queue() const { return m_admin_queue; }
  const NvmeQueueDescriptor& io_queue(uint32_t id) const { return m_io_queues[id]; }

  fk::core::Result<void, fk::core::Error> submit_command(const void* command, uint32_t queue_id);
  fk::core::Result<void, fk::core::Error> process_completions();

private:
  fk::core::Result<uintptr_t, fk::core::Error> allocate_dma_memory(size_t size);
  void free_dma_memory(uintptr_t phys_addr, size_t size);

  NvmeRegisterAccess& m_registers;
  NvmeQueueDescriptor m_admin_queue;
  NvmeQueueDescriptor m_io_queues[8];
  bool m_interrupts_enabled{false};
};

} // namespace fkernel
