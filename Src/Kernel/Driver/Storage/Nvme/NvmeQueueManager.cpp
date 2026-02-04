#include <Kernel/Driver/Storage/Nvme/NvmeQueueManager.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibC/stdlib.h>
#include <LibC/string.h>

namespace fkernel {

NvmeQueueManager::NvmeQueueManager(NvmeRegisterAccess& registers) : m_registers(registers) {
  // Initialize queues
  memset(&m_admin_queue, 0, sizeof(Queue));
  for (auto& io_queue : m_io_queues) {
    memset(&io_queue, 0, sizeof(Queue));
  }
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::setup_admin_queue() {
  fk::algorithms::klog("NVMe-QM", "Setting up admin queues");

  // Setup admin queue attributes (64 entries each)
  // uint32_t aqa = (63 << 16) | 63; // ACQS (15:16) = 63, ASQS (0:15) = 63
  m_registers.write_aqa((63 << 16) | 63);

  // Allocate admin queue memory via MemoryManager façade
  auto admin_sq_res = allocate_dma_memory(4096);
  if (admin_sq_res.is_error())
    return admin_sq_res.error();
  uintptr_t admin_sq_addr = admin_sq_res.value();

  auto admin_cq_res = allocate_dma_memory(4096);
  if (admin_cq_res.is_error()) {
    free_dma_memory(admin_sq_addr, 4096);
    return admin_cq_res.error();
  }
  uintptr_t admin_cq_addr = admin_cq_res.value();

  // Set queue addresses
  m_registers.write_asq(admin_sq_addr);
  m_registers.write_acq(admin_cq_addr);

  // Initialize admin queue structures
  m_admin_queue.memory = reinterpret_cast<void*>(admin_sq_addr);
  m_admin_queue.size = 64;
  m_admin_queue.head = 0;
  m_admin_queue.tail = 0;
  m_admin_queue.phase = true;
  m_admin_queue.phys_addr = (uint32_t)admin_sq_addr;

  m_admin_queue.cq_memory = reinterpret_cast<void*>(admin_cq_addr);
  m_admin_queue.cq_size = 64;
  m_admin_queue.cq_head = 0;
  m_admin_queue.cq_phase = true;
  m_admin_queue.cq_phys_addr = (uint32_t)admin_cq_addr;

  fk::algorithms::klog("NVMe-QM", "Admin queues setup completed");
  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::setup_io_queue(uint32_t queue_id) {
  if (queue_id >= 8) {
    return fk::core::Error::InvalidParameter;
  }

  fk::algorithms::klog("NVMe-QM", "Setting up I/O queue %u", queue_id);

  // Setup I/O queue attributes (32 entries each)
  // uint32_t aqa = (31 << 16) | 31; // ACQS (15:16) = 31, ASQS (0:15) = 31
  // For I/O queues, we need to write to specific registers
  // This is a simplified implementation

  // Allocate I/O queue memory
  auto sq_res = allocate_dma_memory(4096);
  if (sq_res.is_error())
    return sq_res.error();
  uintptr_t sq_addr = sq_res.value();

  auto cq_res = allocate_dma_memory(4096);
  if (cq_res.is_error()) {
    free_dma_memory(sq_addr, 4096);
    return cq_res.error();
  }
  uintptr_t cq_addr = cq_res.value();

  // Set queue addresses
  // In a real implementation, we would write to doorbell registers
  // For now, we just store the addresses
  m_io_queues[queue_id].memory = reinterpret_cast<void*>(sq_addr);
  m_io_queues[queue_id].size = 32;
  m_io_queues[queue_id].head = 0;
  m_io_queues[queue_id].tail = 0;
  m_io_queues[queue_id].phase = true;
  m_io_queues[queue_id].phys_addr = (uint32_t)sq_addr;

  m_io_queues[queue_id].cq_memory = reinterpret_cast<void*>(cq_addr);
  m_io_queues[queue_id].cq_size = 32;
  m_io_queues[queue_id].cq_head = 0;
  m_io_queues[queue_id].cq_phase = true;
  m_io_queues[queue_id].cq_phys_addr = (uint32_t)cq_addr;

  fk::algorithms::klog("NVMe-QM", "I/O queue %u setup completed", queue_id);
  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::reset_controller() {
  fk::algorithms::klog("NVMe-QM", "Resetting controller");

  // Disable controller
  uint32_t cc = m_registers.read_cc();
  cc &= ~0x01; // Clear ENABLE
  m_registers.write_cc(cc);

  // Wait for controller to be ready
  int timeout = 1000;
  while (timeout-- > 0) {
    uint32_t csts = m_registers.read_csts();
    if ((csts & 0x01) == 0) { // RDY bit cleared
      break;
    }
  }

  if (timeout <= 0) {
    fk::algorithms::kerror("NVMe-QM", "Controller disable timeout");
    return fk::core::Error::DeviceError;
  }

  // Set CC with basic configuration
  cc = (0 << 7) | (0 << 4) | (0 << 11); // MPS=0, CSS=NVM, AMS=RR
  m_registers.write_cc(cc);

  // Enable controller
  cc |= 0x01; // Enable
  m_registers.write_cc(cc);

  // Wait for controller to be ready
  timeout = 1000;
  while (timeout-- > 0) {
    uint32_t csts = m_registers.read_csts();
    if ((csts & 0x01) == 0x01) { // RDY set
      fk::algorithms::klog("NVMe-QM", "Controller reset completed");
      return {};
    }
  }

  fk::algorithms::kerror("NVMe-QM", "Controller enable timeout");
  return fk::core::Error::DeviceError;
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::enable_interrupts() {
  // Clear any pending interrupts
  m_registers.write_intms(0xFFFFFFFF);

  m_interrupts_enabled = true;
  fk::algorithms::klog("NVMe-QM", "Interrupts enabled");

  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::submit_command(const void* /*command*/,
                                                                         uint32_t queue_id) {
  if (queue_id >= 8) {
    return fk::core::Error::InvalidParameter;
  }

  // In a real implementation, we would write to the doorbell register
  // For now, we just simulate command submission
  fk::algorithms::klog("NVMe-QM", "Command submitted to queue %u", queue_id);

  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::process_completions() {
  // Process admin completions
  volatile NvmeCompletion* cq = reinterpret_cast<volatile NvmeCompletion*>(m_admin_queue.cq_memory);

  while (true) {
    NvmeCompletion completion;
    memcpy(&completion, const_cast<NvmeQueueManager::NvmeCompletion*>(&cq[m_admin_queue.cq_head]),
           sizeof(NvmeQueueManager::NvmeCompletion));

    if ((completion.status & 0x1) != (m_admin_queue.cq_phase ? 1 : 0)) {
      break;
    }

    m_admin_queue.cq_head = (m_admin_queue.cq_head + 1) % m_admin_queue.cq_size;
    if (m_admin_queue.cq_head == 0) {
      m_admin_queue.cq_phase = !m_admin_queue.cq_phase;
    }
  }

  // Ring admin completion queue doorbell
  // In a real implementation, we would write to the doorbell register
  return {};
}

fk::core::Result<uintptr_t, fk::core::Error> NvmeQueueManager::allocate_dma_memory(size_t size) {
  uintptr_t addr = MemoryManager::the().allocate_contiguous((size + 4095) / 4096);
  if (!addr)
    return fk::core::Error::OutOfMemory;
  return addr;
}

void NvmeQueueManager::free_dma_memory(uintptr_t phys_addr, size_t size) {
  MemoryManager::the().free_contiguous(phys_addr, (size + 4095) / 4096);
}

} // namespace fkernel