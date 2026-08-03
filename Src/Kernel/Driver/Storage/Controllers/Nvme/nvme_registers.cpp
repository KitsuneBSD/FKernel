#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_queue_manager.h>
#include <Kernel/Driver/Storage/Controllers/Nvme/nvme_register_access.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/Logging/log.h>

namespace fkernel {

NvmeRegisterAccess::NvmeRegisterAccess(uintptr_t controller_phys_addr)
    : m_registers(controller_phys_addr ? reinterpret_cast<volatile uint8_t*>(controller_phys_addr) : nullptr),
      m_page_size(4096) {}

uint64_t NvmeRegisterAccess::read_capabilities() const {
  if (!m_registers) { fk::algorithms::kwarn("NVME_REG", "read_capabilities: registers not mapped"); return 0; }
  return *reinterpret_cast<const volatile uint64_t*>(m_registers);
}
uint32_t NvmeRegisterAccess::read_version() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint32_t*>(m_registers + 4);
}
uint32_t NvmeRegisterAccess::read_cc() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint32_t*>(m_registers + 0x14);
}
void NvmeRegisterAccess::write_cc(uint32_t value) {
  if (!m_registers) return;
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x14) = value;
}
uint32_t NvmeRegisterAccess::read_csts() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint32_t*>(m_registers + 0x1C);
}
uint32_t NvmeRegisterAccess::read_aqa() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint32_t*>(m_registers + 0x24);
}
void NvmeRegisterAccess::write_aqa(uint32_t value) {
  if (!m_registers) return;
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0x24) = value;
}
uint64_t NvmeRegisterAccess::read_asq() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint64_t*>(m_registers + 0x28);
}
void NvmeRegisterAccess::write_asq(uint64_t value) {
  if (!m_registers) return;
  *reinterpret_cast<volatile uint64_t*>(m_registers + 0x28) = value;
}
uint64_t NvmeRegisterAccess::read_acq() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint64_t*>(m_registers + 0x30);
}
void NvmeRegisterAccess::write_acq(uint64_t value) {
  if (!m_registers) return;
  *reinterpret_cast<volatile uint64_t*>(m_registers + 0x30) = value;
}
uint32_t NvmeRegisterAccess::read_intms() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint32_t*>(m_registers + 0xE8);
}
void NvmeRegisterAccess::write_intms(uint32_t value) {
  if (!m_registers) return;
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0xE8) = value;
}
uint32_t NvmeRegisterAccess::read_intmc() const {
  if (!m_registers) return 0;
  return *reinterpret_cast<const volatile uint32_t*>(m_registers + 0xEC);
}
void NvmeRegisterAccess::write_intmc(uint32_t value) {
  if (!m_registers) return;
  *reinterpret_cast<volatile uint32_t*>(m_registers + 0xEC) = value;
}
void NvmeRegisterAccess::write_cmd_register(uint8_t, uint32_t, uint64_t, uint64_t) {}

NvmeQueueManager::NvmeQueueManager(NvmeRegisterAccess& registers) : m_registers(registers) {}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::setup_admin_queue() {
  m_admin_queue.size = 32;
  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::setup_io_queue(uint32_t) {
  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::reset_controller() {
  m_registers.write_cc(0);
  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::enable_interrupts() {
  m_interrupts_enabled = true;
  m_registers.write_intmc(0);
  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::submit_command(const void*, uint32_t) {
  return {};
}

fk::core::Result<void, fk::core::Error> NvmeQueueManager::process_completions() {
  return {};
}

fk::core::Result<uintptr_t, fk::core::Error> NvmeQueueManager::allocate_dma_memory(size_t) {
  return 0;
}

void NvmeQueueManager::free_dma_memory(uintptr_t, size_t) {}

} // namespace fkernel