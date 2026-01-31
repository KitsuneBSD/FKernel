#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/tick_manager.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Driver/Storage/Nvme/interrupt_driven_nvme.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Scheduler/scheduler.h>
#include <LibFK/Algorithms/log.h>
#include <LibC/string.h>

namespace fkernel {

// Static storage for interrupt handlers (simple mapping from vector to controller)
static InterruptDrivenNvmeController* s_nvme_controllers[MAX_x86_64_IDT_SIZE] = { nullptr };

static void nvme_interrupt_dispatcher(uint8_t vector, InterruptFrame*) {
  if (s_nvme_controllers[vector]) {
    s_nvme_controllers[vector]->handle_interrupt();
  }
}

// NVMe Register offsets (simplified)
#define NVME_CAP 0x00
#define NVME_VS 0x08
#define NVME_INTMS 0x0C
#define NVME_INTMC 0x10
#define NVME_CC 0x14
#define NVME_CSTS 0x1C
#define NVME_NSSR 0x20
#define NVME_AQA 0x24
#define NVME_ASQ 0x28
#define NVME_ACQ 0x30

// NVMe Commands
#define NVME_CMD_FLUSH 0x00
#define NVME_CMD_WRITE 0x01
#define NVME_CMD_READ 0x02

// Admin Commands
#define NVME_ADMIN_DELETE_SQ 0x00
#define NVME_ADMIN_CREATE_SQ 0x01
#define NVME_ADMIN_IDENTIFY 0x06

InterruptDrivenNvmeController* InterruptDrivenNvmeController::create(const PciDevice& device) {
  fk::algorithms::klog(
      "NVMe-INT", "Creating interrupt-driven NVMe controller for device %02x:%02x.%d",
      device.address().bus(), device.address().device(), device.address().function());

  auto controller_result = fk::make_ref<InterruptDrivenNvmeController>(device);
  if (controller_result.is_error()) {
    fk::algorithms::kerror("NVMe-INT", "Failed to create controller instance");
    return nullptr;
  }

  auto controller = controller_result.value();
  auto init_result = controller->initialize_interrupt_driven();
  if (init_result.is_error()) {
    fk::algorithms::kerror("NVMe-INT", "Failed to initialize controller: %d",
                           (int)init_result.error());
    return nullptr;
  }

  fk::algorithms::klog("NVMe-INT", "Interrupt-driven controller created successfully");
  return controller.leak_ptr();
}

InterruptDrivenNvmeController::~InterruptDrivenNvmeController() {
    uint8_t vector = m_interrupt_line + 32;
    if (s_nvme_controllers[vector] == this) {
        s_nvme_controllers[vector] = nullptr;
    }
}

fk::core::Result<void, fk::core::Error>
InterruptDrivenNvmeController::initialize_interrupt_driven() {
  // Map PCI BAR0 (NVMe controller registers)
  uint32_t bar0 = PciManager::the().read_config_dword(m_pci_device.address(), 0x10);
  if ((bar0 & 0x01) != 0) { // I/O space - not supported for NVMe
    fk::algorithms::kerror("NVMe-INT", "IO space BAR not supported");
    return fk::core::Error::InvalidParameter;
  }

  uintptr_t controller_phys_addr = bar0 & ~0xFu; // Clear bits 0-3

  // Map controller registers via MemoryManager façade
  MemoryManager::the().map_page(controller_phys_addr, controller_phys_addr,
                                PageFlags::Present | PageFlags::Writable |
                                    PageFlags::CacheDisabled);

  m_controller_regs = reinterpret_cast<volatile uint8_t*>(controller_phys_addr);
  fk::algorithms::klog("NVMe-INT", "Controller registers mapped at %p",
                       (void*)controller_phys_addr);

  // Read NVMe capabilities
  uint64_t cap = *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_CAP);
  uint32_t vs = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_VS);

  m_controller_page_size = 4096 << ((cap >> 0) & 0xF); // MPS (Memory Page Size)

  fk::algorithms::klog("NVMe-INT", "NVMe version %d.%d, page size: %d bytes", (vs >> 16) & 0xFFFF,
                       vs & 0xFFFF, m_controller_page_size);

  // Enable bus mastering
  uint32_t command = PciManager::the().read_config_dword(m_pci_device.address(), 0x04);
  command |= 0x04; // Bus Master Enable
  command |= 0x02; // Memory Space Enable
  PciManager::the().write_config_dword(m_pci_device.address(), 0x04, command);

  // Reset controller
  auto reset_result = reset_controller();
  if (reset_result.is_error()) {
    return reset_result.error();
  }

  // Setup admin queues
  auto admin_result = setup_admin_queue();
  if (admin_result.is_error()) {
    return admin_result.error();
  }

  // Setup I/O queues
  auto io_result = setup_io_queues();
  if (io_result.is_error()) {
    return io_result.error();
  }

  // Identify namespace
  auto ns_result = identify_namespace();
  if (ns_result.is_error()) {
    return ns_result.error();
  }

  // Enable interrupts
  auto interrupt_result = enable_interrupts();
  if (interrupt_result.is_error()) {
    return interrupt_result.error();
  }

  // Register interrupt handler
  NvmeInterruptHandler::register_handler(fk::RefPtr<InterruptDrivenNvmeController>(this));

  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::reset_controller() {
  fk::algorithms::klog("NVMe-INT", "Resetting controller");

  // Disable controller
  uint32_t cc = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC);
  cc &= ~0x01; // Clear ENABLE
  *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC) = cc;

  // Wait for controller to be ready
  int timeout = 1000;
  while (timeout-- > 0) {
    uint32_t csts = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CSTS);
    if ((csts & 0x01) == 0) { // RDY bit cleared
      break;
    }
  }

  if (timeout <= 0) {
    fk::algorithms::kerror("NVMe-INT", "Controller disable timeout");
    return fk::core::Error::DeviceError;
  }

  // Set CC with basic configuration
  cc = (0 << 7) | (0 << 4) | (0 << 11); // MPS=0, CSS=NVM, AMS=RR
  *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC) = cc;

  // Enable controller
  cc |= 0x01; // Enable
  *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC) = cc;

  // Wait for controller to be ready
  timeout = 1000;
  while (timeout-- > 0) {
    uint32_t csts = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CSTS);
    if ((csts & 0x01) == 0x01) { // RDY set
      fk::algorithms::klog("NVMe-INT", "Controller reset completed");
      return {};
    }
  }

  fk::algorithms::kerror("NVMe-INT", "Controller enable timeout");
  return fk::core::Error::DeviceError;
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::setup_admin_queue() {
  fk::algorithms::klog("NVMe-INT", "Setting up admin queues");

  // Setup admin queue attributes (64 entries each)
  uint32_t aqa = (63 << 16) | 63; // ACQS (15:16) = 63, ASQS (0:15) = 63
  *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_AQA) = aqa;

  // Allocate admin queue memory via MemoryManager façade
  auto admin_sq_res = allocate_dma_memory(4096);
  if (admin_sq_res.is_error()) return admin_sq_res.error();
  uintptr_t admin_sq_addr = admin_sq_res.value();

  auto admin_cq_res = allocate_dma_memory(4096);
  if (admin_cq_res.is_error()) return admin_cq_res.error();
  uintptr_t admin_cq_addr = admin_cq_res.value();

  // Map memory
  MemoryManager::the().map_page(admin_sq_addr, admin_sq_addr, 
                                PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
  MemoryManager::the().map_page(admin_cq_addr, admin_cq_addr, 
                                PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);

  // Clear memory
  memset(reinterpret_cast<void*>(admin_sq_addr), 0, 4096);
  memset(reinterpret_cast<void*>(admin_cq_addr), 0, 4096);

  // Set queue addresses
  *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_ASQ) = admin_sq_addr;
  *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_ACQ) = admin_cq_addr;

  // Initialize admin queue structures
  m_admin_sq.sq_memory = reinterpret_cast<void*>(admin_sq_addr);
  m_admin_sq.sq_size = 64;
  m_admin_sq.sq_head = 0;
  m_admin_sq.sq_tail = 0;
  m_admin_sq.sq_phase = true;
  m_admin_sq.sq_phys_addr = (uint32_t)admin_sq_addr;

  m_admin_cq.cq_memory = reinterpret_cast<void*>(admin_cq_addr);
  m_admin_cq.cq_size = 64;
  m_admin_cq.cq_head = 0;
  m_admin_cq.cq_phase = true;
  m_admin_cq.cq_phys_addr = (uint32_t)admin_cq_addr;

  return {};
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::enable_interrupts() {
  // Get PCI interrupt line
  m_interrupt_line = PciManager::the().read_config_byte(m_pci_device.address(), 0x3C);

  // Clear any pending interrupts
  *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_INTMS) = 0xFFFFFFFF;

  m_interrupts_enabled = true;
  fk::algorithms::klog("NVMe-INT", "Interrupts enabled on IRQ %d", m_interrupt_line);

  return {};
}

void InterruptDrivenNvmeController::handle_interrupt() {
  if (!m_interrupts_enabled)
    return;

  // Clear interrupt
  *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_INTMC) = 0xFFFFFFFF;

  // Process completions from admin queue
  process_completions();

  // Process completions from I/O queues
  for (uint32_t i = 0; i < 8; ++i) {
    if (m_io_cq[i].cq_memory) {
      // Process I/O queue completions
      volatile NvmeCompletion* cq =
          reinterpret_cast<volatile NvmeCompletion*>(m_io_cq[i].cq_memory);
      
      while (true) {
        NvmeCompletion completion;
        memcpy(&completion, (void*)&cq[m_io_cq[i].cq_head], sizeof(NvmeCompletion));

        if ((completion.status & 0x1) != (m_io_cq[i].cq_phase ? 1 : 0)) {
           break;
        }

        uint16_t command_id = completion.command_id;
        IoCompletionStatus status = IoCompletionStatus::Success;

        if ((completion.status >> 1) != 0) {
          fk::algorithms::kerror("NVMe-INT", "Command %d failed with status 0x%x", command_id,
                                 completion.status >> 1);
          status = IoCompletionStatus::Error;
        }

        complete_operation(command_id, status);

        // Update completion queue head
        m_io_cq[i].cq_head = (m_io_cq[i].cq_head + 1) % m_io_cq[i].cq_size;
        if (m_io_cq[i].cq_head == 0) {
          m_io_cq[i].cq_phase = !m_io_cq[i].cq_phase;
        }
      }

      // Ring completion queue doorbell
      *m_io_cq[i].cq_head_db = m_io_cq[i].cq_head;
    }
  }
}

fk::core::Result<NvmeAsyncOperation*, fk::core::Error>
InterruptDrivenNvmeController::submit_read_async(uint64_t start_lba, uint32_t block_count,
                                                 uint8_t* buffer) {
  uint16_t command_id = find_free_command_id();
  if (command_id == 0xFFFF) {
    return fk::core::Error::DeviceBusy;
  }

  mark_command_id_used(command_id);

  // Create async operation
  auto operation = new NvmeAsyncOperation(command_id, start_lba, block_count, buffer, false);
  if (!operation) {
    mark_command_id_free(command_id);
    return fk::core::Error::OutOfMemory;
  }

  m_pending_operations[command_id] = operation;

  // Setup PRP list via MemoryManager façade
  uint64_t prp1 = MemoryManager::the().translate(reinterpret_cast<uintptr_t>(buffer));
  uint64_t prp2 = 0;

  if (block_count * m_block_size > 4096) {
    fk::algorithms::kerror("NVMe-INT", "Large transfers not yet supported");
    mark_command_id_free(command_id);
    return fk::core::Error::NotImplemented;
  }

  // Build read command
  NvmeCommand cmd = build_read_command(start_lba, block_count, prp1, prp2);
  cmd.cdw0 |= (command_id & 0xFFFF) << 16; 

  // Submit command to I/O queue 1
  auto submit_result = submit_io_command(cmd, 1);
  if (submit_result.is_error()) {
    mark_command_id_free(command_id);
    return submit_result.error();
  }

  fk::algorithms::klog("NVMe-INT", "Submitted async read: command_id=%d, lba=%ld, count=%d",
                       command_id, start_lba, block_count);

  return operation;
}

fk::core::Result<size_t, fk::core::Error>
InterruptDrivenNvmeController::read_blocks(uint64_t start_lba, size_t count, uint8_t* buffer) {
  auto async_op_res = submit_read_async(start_lba, (uint32_t)count, buffer);
  if (async_op_res.is_error()) {
    return async_op_res.error();
  }

  auto async_op = async_op_res.value();

  // Wait for completion
  IoCompletionStatus status = async_op->wait_for_completion(5000); // 5 second timeout

  if (status == IoCompletionStatus::Success) {
    return count;
  } else if (status == IoCompletionStatus::Timeout) {
    return fk::core::Error::Timeout;
  } else {
    return fk::core::Error::DeviceError;
  }
}

void InterruptDrivenNvmeController::process_completions() {
  volatile NvmeCompletion* cq = reinterpret_cast<volatile NvmeCompletion*>(m_admin_cq.cq_memory);
  
  while (true) {
    NvmeCompletion completion;
    memcpy(&completion, (void*)&cq[m_admin_cq.cq_head], sizeof(NvmeCompletion));

    if ((completion.status & 0x1) != (m_admin_cq.cq_phase ? 1 : 0)) {
       break;
    }

    m_admin_cq.cq_head = (m_admin_cq.cq_head + 1) % m_admin_cq.cq_size;
    if (m_admin_cq.cq_head == 0) {
      m_admin_cq.cq_phase = !m_admin_cq.cq_phase;
    }
  }

  // Ring admin completion queue doorbell
  *m_admin_cq.cq_head_db = m_admin_cq.cq_head;
}

void InterruptDrivenNvmeController::complete_operation(uint16_t command_id,
                                                       IoCompletionStatus status) {
  if (command_id >= m_pending_operations.size())
    return;

  auto& operation = m_pending_operations[command_id];
  if (!operation)
    return;

  if (status == IoCompletionStatus::Success) {
    operation->mark_success();
  } else {
    operation->mark_error();
  }

  // Mark command ID as free
  mark_command_id_free(command_id);
}

NvmeCommand InterruptDrivenNvmeController::build_read_command(uint64_t lba, uint32_t block_count,
                                                              uint64_t prp1, uint64_t prp2) {
  NvmeCommand cmd = {};
  cmd.cdw0 = NVME_CMD_READ;
  cmd.nsid = m_namespace_id;
  cmd.prp1 = prp1;
  cmd.prp2 = prp2;
  cmd.cdw10 = (uint32_t)lba;         // Starting LBA
  cmd.cdw11 = (uint32_t)(lba >> 32); // Starting LBA high
  cmd.cdw12 = block_count - 1;       // Number of blocks (0-based)
  return cmd;
}

NvmeCommand InterruptDrivenNvmeController::build_write_command(uint64_t lba, uint32_t block_count,
                                                               uint64_t prp1, uint64_t prp2) {
  NvmeCommand cmd = {};
  cmd.cdw0 = NVME_CMD_WRITE;
  cmd.nsid = m_namespace_id;
  cmd.prp1 = prp1;
  cmd.prp2 = prp2;
  cmd.cdw10 = (uint32_t)lba;         // Starting LBA
  cmd.cdw11 = (uint32_t)(lba >> 32); // Starting LBA high
  cmd.cdw12 = block_count - 1;       // Number of blocks (0-based)
  return cmd;
}

uint16_t InterruptDrivenNvmeController::find_free_command_id() {
  for (uint16_t id = 1; id < m_pending_operations.size(); ++id) {
    if (!m_pending_operations[id]) {
      return id;
    }
  }
  return 0xFFFF; // No free command IDs
}

void InterruptDrivenNvmeController::mark_command_id_used(uint16_t) {}

void InterruptDrivenNvmeController::mark_command_id_free(uint16_t command_id) {
  if (command_id < m_pending_operations.size()) {
    m_pending_operations[command_id] = nullptr;
  }
}

void NvmeInterruptHandler::register_handler(fk::RefPtr<InterruptDrivenNvmeController> controller) {
  uint32_t irq = controller->get_interrupt_line();
  if (irq == 0 || irq >= 32) {
    fk::algorithms::kerror("NVMe-INT", "Invalid IRQ line: %d", irq);
    return;
  }

  uint8_t vector = irq + 32;
  s_nvme_controllers[vector] = controller.ptr();

  // Register with interrupt controller
  InterruptController::the().register_interrupt(nvme_interrupt_dispatcher, vector);

  fk::algorithms::klog("NVMe-INT", "Registered interrupt handler for IRQ %d (vector %d)", irq, vector);
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::setup_io_queues() {
    return {}; // Stub para o linker
}

fk::core::Result<void, fk::core::Error> InterruptDrivenNvmeController::identify_namespace() {
    return {}; // Stub para o linker
}

fk::core::Result<uintptr_t, fk::core::Error> InterruptDrivenNvmeController::allocate_dma_memory(size_t size) {
    uintptr_t addr = MemoryManager::the().allocate_contiguous((size + 4095) / 4096);
    if (!addr) return fk::core::Error::OutOfMemory;
    return addr;
}

fk::core::Result<uint16_t, fk::core::Error> InterruptDrivenNvmeController::submit_io_command(const NvmeCommand&, uint32_t) {
    return (uint16_t)0; // Stub para o linker
}

void InterruptDrivenNvmeController::free_dma_memory(uintptr_t phys_addr, size_t size) {
    MemoryManager::the().free_contiguous(phys_addr, (size + 4095) / 4096);
}

} // namespace fkernel