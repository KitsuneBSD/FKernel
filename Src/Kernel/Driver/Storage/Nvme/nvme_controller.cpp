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
    
    // Enable controller with IO queues
    cc = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC);
    cc = (cc & ~0x06) | 0x00; // Clear IO submission/completion queue size, use default
    cc |= 0x01; // Enable
    *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CC) = cc;
    
    // Wait for controller to be ready
    timeout = 1000;
    while (timeout-- > 0) {
        uint32_t csts = *reinterpret_cast<volatile uint32_t*>(m_controller_regs + NVME_CSTS);
        if ((csts & 0x01) == 0x01 && (csts & 0x02) == 0) { // RDY set and CSTS cleared
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
    
    // Clear queue memory
    memset(m_admin_queue.sq_memory, 0, 4096);
    memset(m_admin_queue.cq_memory, 0, 4096);
    
    // Set admin queue base addresses
    *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_ASQ) = admin_sq_addr;
    *reinterpret_cast<volatile uint64_t*>(m_controller_regs + NVME_ACQ) = admin_cq_addr;
    
    // Setup doorbell pointers
    uint32_t doorbell_offset = 0x1000; // Base doorbell offset
    m_admin_queue.sq_tail_db = reinterpret_cast<volatile uint32_t*>(m_controller_regs + doorbell_offset);
    m_admin_queue.cq_head_db = reinterpret_cast<volatile uint32_t*>(m_controller_regs + doorbell_offset + m_doorbell_stride);
    
    fk::algorithms::klog("NVMe", "Admin queue configured (SQ:%p, CQ:%p)", 
                         (void*)admin_sq_addr, (void*)admin_cq_addr);
}

fk::core::Result<void, fk::core::Error> NVMeController::identify_controller() {
    // TODO: Send IDENTIFY command to get controller information
    // For now, just log placeholder
    fk::algorithms::klog("NVMe", "Controller identify (placeholder)");
    return {};
}

void NVMeController::scan_namespaces() {
    // TODO: Send IDENTIFY command for each namespace
    // For now, create placeholder namespace
    Namespace ns;
    ns.nsid = 1;
    ns.size_blocks = 1024 * 1024; // 1M blocks placeholder
    ns.block_size = 4096; // 4KB blocks
    ns.active = true;
    
    m_namespaces.push_back(ns);
    
    fk::algorithms::klog("NVMe", "Namespace scan complete. Found %u namespace(s)", m_namespaces.size());
    for (const auto& ns : m_namespaces) {
        fk::algorithms::klog("NVMe", "Namespace %u: %llu blocks x %u bytes = %llu MB", 
                             ns.nsid, ns.size_blocks, ns.block_size, 
                             (ns.size_blocks * ns.block_size) / (1024 * 1024));
    }
}

void NVMeController::probe() {
    fk::algorithms::klog("NVMe", "Probing NVMe controller...");
    
    if (!m_initialized) {
        fk::algorithms::kwarn("NVMe", "Controller not initialized, skipping probe");
        return;
    }
    
    // TODO: Initialize namespace devices
    // This would involve creating block device nodes for each namespace
    
    fk::algorithms::klog("NVMe", "Probe completed");
}

fk::core::Result<size_t, fk::core::Error> 
NVMeController::read([[maybe_unused]] uint64_t offset, [[maybe_unused]] size_t size, [[maybe_unused]] uint8_t* buffer) {
    // TODO: Implement NVMe read operations
    // For now, return not implemented
    return fk::core::Error::NotImplemented;
}

fk::core::Result<size_t, fk::core::Error> 
NVMeController::write([[maybe_unused]] uint64_t offset, [[maybe_unused]] size_t size, [[maybe_unused]] const uint8_t* buffer) {
    // TODO: Implement NVMe write operations  
    // For now, return not implemented
    return fk::core::Error::NotImplemented;
}

size_t NVMeController::size() const {
    // TODO: Calculate total size of all namespaces
    return 0;
}

} // namespace fkernel