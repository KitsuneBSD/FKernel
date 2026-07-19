#include <Kernel/Driver/Storage/Nvme/nvme_controller.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

fk::RefPtr<NVMeController> NVMeController::create(const PciDevice& device) {
    fk::algorithms::klog("NVMe", "Creating controller for device %02x:%02x.%d (Vendor:%04x Device:%04x)",
                         device.address().bus(), device.address().device(), 
                         device.address().function(), device.vendor_id(), device.device_id());
    
    auto controller_result = fk::make_ref<NVMeController>(device);
    if (controller_result.is_error()) {
        fk::algorithms::kerror("NVMe", "Failed to create controller instance");
        return nullptr;
    }
    
    auto controller = controller_result.value();
    auto init_result = controller->initialize_controller();
    if (init_result.is_error()) {
        fk::algorithms::kerror("NVMe", "Failed to initialize controller: %d", (int)init_result.error());
        return nullptr;
    }
    
    fk::algorithms::klog("NVMe", "Controller created and initialized successfully");
    return controller;
}

NVMeController::NVMeController(const PciDevice& device) 
    : m_pci_device(device) {
    set_name("nvme0");
}

NVMeController::~NVMeController() {
    if (m_controller_regs) {
        // Disable controller
        uint32_t cc = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC);
        cc &= ~0x01; // Clear ENABLE
        *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC) = cc;
        
        fk::algorithms::klog("NVMe", "Controller disabled and destroyed");
    }
}

fk::core::Result<void, fk::core::Error> NVMeController::initialize_controller() {
    // Map PCI BAR0 (NVMe controller registers)
    uint32_t bar0 = PciManager::the().read_config_dword(m_pci_device.address(), 0x10);
    if ((bar0 & 0x01) == 0) { // Memory mapped
        uint64_t controller_phys_addr = bar0 & ~0xFu; // Clear bits 0-3
        if (controller_phys_addr == 0) {
            // Check for 64-bit BAR
            uint32_t bar1 = PciManager::the().read_config_dword(m_pci_device.address(), 0x14);
            controller_phys_addr |= (uint64_t)bar1 << 32;
        }
        
        // Map the controller registers
        MemoryManager::the().map_page(controller_phys_addr, controller_phys_addr, 
                                     PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
        
        m_controller_regs = reinterpret_cast<volatile uint8_t*>(controller_phys_addr);
        fk::algorithms::klog("NVMe", "Controller registers mapped at %p", (void*)(uintptr_t)controller_phys_addr);
    } else {
        fk::algorithms::kerror("NVMe", "IO space BAR not supported");
        return fk::core::Error::InvalidParameter;
    }
    
    // Read NVMe capabilities
    m_controller_capabilities = *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_CAP);
    m_version = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_VS);
    m_doorbell_stride = ((m_controller_capabilities >> 32) & 0xF) + 1; // DSTRD field + 1
    
    fk::algorithms::klog("NVMe", "NVMe version %d.%d, capabilities: 0x%016llx, doorbell stride: %u", 
                         (m_version >> 16) & 0xFFFF, m_version & 0xFFFF, 
                         m_controller_capabilities, m_doorbell_stride);
    
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
    
    // Configure admin queues
    configure_admin_queue();
    
    // Create IO queues
    auto io_queue_result = create_io_queues();
    if (io_queue_result.is_error()) {
        return io_queue_result.error();
    }
    
    // Identify controller
    auto identify_result = identify_controller();
    if (identify_result.is_error()) {
        return identify_result.error();
    }
    
    // Scan namespaces
    scan_namespaces();
    
    m_initialized = true;
    return {};
}

fk::core::Result<void, fk::core::Error> NVMeController::reset_controller() {
    fk::algorithms::klog("NVMe", "Resetting controller...");
    
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
        fk::algorithms::kerror("NVMe", "Controller disable timeout");
        return fk::core::Error::DeviceError;
    }
    
    // Set CC with basic configuration (MPS=0, CSS=NVM, AMS=RR)
    cc = (0 << 7) | (0 << 4) | (0 << 11); 
    *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC) = cc;

    // Enable controller
    cc |= 0x01; // Enable
    *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC) = cc;
    
    // Wait for controller to be ready
    timeout = 1000;
    while (timeout-- > 0) {
        uint32_t csts = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CSTS);
        if ((csts & 0x01) == 0x01) { // RDY set
            fk::algorithms::klog("NVMe", "Controller reset completed");
            return {};
        }
    }
    
    fk::algorithms::kerror("NVMe", "Controller enable timeout");
    return fk::core::Error::DeviceError;
}

void NVMeController::configure_admin_queue() {
    // Setup admin queue attributes (64 entries each)
    uint32_t aqa = (63 << 16) | 63; // ACQS (15:16) = 63, ASQS (0:15) = 63
    *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_AQA) = aqa;
    
    // Allocate admin queue memory (4KB each)
    uintptr_t admin_sq_addr = PhysicalMemoryManager::the().alloc_page();
    uintptr_t admin_cq_addr = PhysicalMemoryManager::the().alloc_page();
    
    ASSERT(admin_sq_addr != 0 && admin_cq_addr != 0);
    
    // Map admin queue memory
    MemoryManager::the().map_page(admin_sq_addr, admin_sq_addr, 
                                 PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    MemoryManager::the().map_page(admin_cq_addr, admin_cq_addr, 
                                 PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    
    m_admin_queue.sq_memory = reinterpret_cast<void*>(admin_sq_addr);
    m_admin_queue.cq_memory = reinterpret_cast<void*>(admin_cq_addr);
    m_admin_queue.sq_size = 64;
    m_admin_queue.cq_size = 64;
    m_admin_queue.sq_tail = 0;
    m_admin_queue.cq_head = 0;
    m_admin_queue.cq_phase = 1;
    
    // Clear queue memory
    memset(m_admin_queue.sq_memory, 0, 4096);
    memset(m_admin_queue.cq_memory, 0, 4096);
    
    // Set admin queue base addresses
    *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_ASQ) = admin_sq_addr;
    *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_ACQ) = admin_cq_addr;
    
    // Setup doorbell pointers
    uint32_t doorbell_offset = 0x1000; // Base doorbell offset
    m_admin_queue.sq_tail_db = reinterpret_cast<volatile uint32_t*>(m_controller_regs + doorbell_offset);
    m_admin_queue.cq_head_db = reinterpret_cast<volatile uint32_t*>(m_controller_regs + doorbell_offset + (1 * m_doorbell_stride * 4));
    
    fk::algorithms::klog("NVMe", "Admin queue configured (SQ:%p, CQ:%p)", 
                         (void*)admin_sq_addr, (void*)admin_cq_addr);
}

fk::core::Result<void, fk::core::Error> NVMeController::create_io_queues() {
    fk::algorithms::klog("NVMe", "Creating IO queues...");

    // Allocate IO queue memory
    uintptr_t io_sq_addr = PhysicalMemoryManager::the().alloc_page();
    uintptr_t io_cq_addr = PhysicalMemoryManager::the().alloc_page();
    
    ASSERT(io_sq_addr != 0 && io_cq_addr != 0);
    
    MemoryManager::the().map_page(io_sq_addr, io_sq_addr, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    MemoryManager::the().map_page(io_cq_addr, io_cq_addr, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    
    m_io_queue.sq_memory = reinterpret_cast<void*>(io_sq_addr);
    m_io_queue.cq_memory = reinterpret_cast<void*>(io_cq_addr);
    m_io_queue.sq_size = 64;
    m_io_queue.cq_size = 64;
    m_io_queue.sq_tail = 0;
    m_io_queue.cq_head = 0;
    m_io_queue.cq_phase = 1;
    
    memset(m_io_queue.sq_memory, 0, 4096);
    memset(m_io_queue.cq_memory, 0, 4096);

    uint32_t doorbell_offset = 0x1000;
    m_io_queue.sq_tail_db = reinterpret_cast<volatile uint32_t*>(m_controller_regs + doorbell_offset + (2 * m_doorbell_stride * 4));
    m_io_queue.cq_head_db = reinterpret_cast<volatile uint32_t*>(m_controller_regs + doorbell_offset + (3 * m_doorbell_stride * 4));

    // 1. Create IO CQ
    Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.cdw0 = NVME_CMD_CREATE_IO_CQ;
    cmd.prp1 = io_cq_addr;
    cmd.cdw10 = ((m_io_queue.cq_size - 1) << 16) | 1; // Size, Queue ID 1
    cmd.cdw11 = 1; // Physically contiguous, interrupts disabled for now

    auto res = submit_command(m_admin_queue, cmd);
    if (res.is_error()) return res.error();

    // 2. Create IO SQ
    memset(&cmd, 0, sizeof(Command));
    cmd.cdw0 = NVME_CMD_CREATE_IO_SQ;
    cmd.prp1 = io_sq_addr;
    cmd.cdw10 = ((m_io_queue.sq_size - 1) << 16) | 1; // Size, Queue ID 1
    cmd.cdw11 = (1 << 16) | 1; // CQ ID 1, Physically contiguous

    res = submit_command(m_admin_queue, cmd);
    if (res.is_error()) return res.error();

    fk::algorithms::klog("NVMe", "IO queues created successfully");
    return {};
}

fk::core::Result<void, fk::core::Error> NVMeController::submit_command(QueuePair& queue, Command& cmd) {
    Command* sq = reinterpret_cast<Command*>(queue.sq_memory);
    sq[queue.sq_tail] = cmd;
    
    queue.sq_tail = (queue.sq_tail + 1) % queue.sq_size;
    *queue.sq_tail_db = queue.sq_tail;

    // Wait for completion (polling)
    volatile Completion* cq = reinterpret_cast<volatile Completion*>(queue.cq_memory);
    int timeout = 1000000;
    while (timeout-- > 0) {
        uint16_t status = cq[queue.cq_head].status;
        if ((status & 0x1) == queue.cq_phase) {
            queue.cq_head = (queue.cq_head + 1) % queue.cq_size;
            if (queue.cq_head == 0) queue.cq_phase = !queue.cq_phase;
            *queue.cq_head_db = queue.cq_head;
            
            if ((status >> 1) != 0) {
                fk::algorithms::kerror("NVMe", "Command failed with status 0x%x", status >> 1);
                return fk::core::Error::DeviceError;
            }
            return {};
        }
    }

    fk::algorithms::kerror("NVMe", "Command timeout");
    return fk::core::Error::DeviceError;
}

fk::core::Result<void, fk::core::Error> NVMeController::identify_controller() {
    fk::algorithms::klog("NVMe", "Controller identify");
    uintptr_t buffer_phys = PhysicalMemoryManager::the().alloc_page();
    MemoryManager::the().map_page(buffer_phys, buffer_phys, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    
    Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.cdw0 = NVME_CMD_IDENTIFY;
    cmd.prp1 = buffer_phys;
    cmd.cdw10 = 1; // Identify Controller

    auto res = submit_command(m_admin_queue, cmd);
    // TODO: Parse identify data
    return res;
}

void NVMeController::scan_namespaces() {
    // Step 1: Identify Active Namespace List (CNS=0x02)
    uintptr_t ns_list_phys = PhysicalMemoryManager::the().alloc_page();
    MemoryManager::the().map_page(ns_list_phys, ns_list_phys,
        PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
    memset(reinterpret_cast<void*>(ns_list_phys), 0, 4096);

    Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.cdw0 = NVME_CMD_IDENTIFY;
    cmd.prp1 = ns_list_phys;
    cmd.cdw10 = 0x02; // CNS=0x02: Active NS ID list

    if (submit_command(m_admin_queue, cmd).is_error()) {
        fk::algorithms::kwarn("NVMe", "Active NS list failed — falling back to NSID=1");
        Namespace ns;
        ns.nsid = 1; ns.size_blocks = 0; ns.block_size = 512; ns.active = true;
        m_namespaces.push_back(ns);
        return;
    }

    auto* ns_ids = reinterpret_cast<uint32_t*>(ns_list_phys);

    // Step 2: Identify each namespace (CNS=0x00)
    uintptr_t ns_data_phys = PhysicalMemoryManager::the().alloc_page();
    MemoryManager::the().map_page(ns_data_phys, ns_data_phys,
        PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);

    for (int i = 0; i < 1024 && ns_ids[i] != 0; ++i) {
        uint32_t nsid = ns_ids[i];
        memset(reinterpret_cast<void*>(ns_data_phys), 0, 4096);

        Command ns_cmd;
        memset(&ns_cmd, 0, sizeof(Command));
        ns_cmd.cdw0 = NVME_CMD_IDENTIFY;
        ns_cmd.nsid = nsid;
        ns_cmd.prp1 = ns_data_phys;
        ns_cmd.cdw10 = 0x00; // CNS=0x00: Identify Namespace

        if (submit_command(m_admin_queue, ns_cmd).is_error()) continue;

        auto* data = reinterpret_cast<uint8_t*>(ns_data_phys);
        uint64_t nsze = *reinterpret_cast<uint64_t*>(data + 0);
        uint8_t flbas = data[26] & 0x0F;
        uint32_t lbaf = *reinterpret_cast<uint32_t*>(data + 128 + flbas * 4);
        uint8_t lbads = (lbaf >> 16) & 0xFF; // log2(block_size)
        uint32_t block_size = lbads ? (1u << lbads) : 512;

        if (nsze == 0) nsze = 1024 * 1024; // fallback if identify returns 0

        Namespace ns;
        ns.nsid = nsid;
        ns.size_blocks = nsze;
        ns.block_size = block_size;
        ns.active = true;
        m_namespaces.push_back(ns);

        fk::algorithms::klog("NVMe", "Found NS %u: %llu blocks * %u bytes",
                              nsid, (unsigned long long)nsze, block_size);
    }

    if (m_namespaces.size() == 0) {
        fk::algorithms::kwarn("NVMe", "No namespaces found — adding NSID=1 as fallback");
        Namespace ns;
        ns.nsid = 1; ns.size_blocks = 1024 * 1024; ns.block_size = 512; ns.active = true;
        m_namespaces.push_back(ns);
    }

    fk::algorithms::klog("NVMe", "scan_namespaces: found %zu namespace(s)", m_namespaces.size());
}

void NVMeController::probe() {
    fk::algorithms::klog("NVMe", "Probing NVMe controller...");
    if (!m_initialized) return;
    fk::algorithms::klog("NVMe", "Probe completed");
}

fk::core::Result<size_t, fk::core::Error> 
NVMeController::read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) {
    if (!m_initialized) return fk::core::Error::DeviceError;
    
    Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.cdw0 = NVME_CMD_READ;
    cmd.nsid = 1;
    cmd.prp1 = (uintptr_t)buffer; // Must be physical address
    cmd.cdw10 = (uint32_t)start_sector;
    cmd.cdw11 = (uint32_t)(start_sector >> 32);
    cmd.cdw12 = (uint32_t)((count - 1) & 0xFFFF);

    auto res = submit_command(m_io_queue, cmd);
    if (res.is_error()) return res.error();
    return count;
}

fk::core::Result<size_t, fk::core::Error> 
NVMeController::write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) {
    if (!m_initialized) return fk::core::Error::DeviceError;

    Command cmd;
    memset(&cmd, 0, sizeof(Command));
    cmd.cdw0 = NVME_CMD_WRITE;
    cmd.nsid = 1;
    cmd.prp1 = (uintptr_t)buffer;
    cmd.cdw10 = (uint32_t)start_sector;
    cmd.cdw11 = (uint32_t)(start_sector >> 32);
    cmd.cdw12 = (uint32_t)((count - 1) & 0xFFFF);

    auto res = submit_command(m_io_queue, cmd);
    if (res.is_error()) return res.error();
    return count;
}

SectorCount NVMeController::sector_count() const {
    if (m_namespaces.is_empty()) return SectorCount(0);
    return SectorCount(m_namespaces[0].size_blocks);
}

fk::core::Result<size_t, fk::core::Error> 
NVMeController::read(uint64_t offset, size_t size, uint8_t* buffer) {
    uint64_t start_sector = offset / 4096;
    size_t count = (size + 4095) / 4096;
    return read_sectors(start_sector, count, buffer);
}

fk::core::Result<size_t, fk::core::Error> 
NVMeController::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    uint64_t start_sector = offset / 4096;
    size_t count = (size + 4095) / 4096;
    return write_sectors(start_sector, count, buffer);
}

size_t NVMeController::size() const {
    if (m_namespaces.is_empty()) return 0;
    return m_namespaces[0].size_blocks * 4096;
}

} // namespace fkernel