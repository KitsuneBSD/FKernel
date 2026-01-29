#include <Kernel/Driver/Storage/Ahci/ahci_controller.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/ref_ptr.h>

namespace fkernel {

fk::RefPtr<AHCIController> AHCIController::create(const PciDevice& device) {
    fk::algorithms::klog("AHCI", "Creating controller for device %02x:%02x.%d (Vendor:%04x Device:%04x)",
                         device.address().bus(), device.address().device(), 
                         device.address().function(), device.vendor_id(), device.device_id());
    
    auto controller_result = fk::make_ref<AHCIController>(device);
    if (controller_result.is_error()) {
        fk::algorithms::kerror("AHCI", "Failed to create controller instance");
        return nullptr;
    }
    
    auto controller = controller_result.value();
    auto init_result = controller->initialize_controller();
    if (init_result.is_error()) {
        fk::algorithms::kerror("AHCI", "Failed to initialize controller: %d", (int)init_result.error());
        return nullptr;
    }
    
    fk::algorithms::klog("AHCI", "Controller created and initialized successfully");
    return controller;
}

AHCIController::AHCIController(const PciDevice& device) 
    : m_pci_device(device) {
    set_name("ahci0");
}

AHCIController::~AHCIController() {
    if (m_hba_base) {
        // Disable controller
        uint32_t ghc = *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC);
        ghc &= ~(1u << 0); // Clear AE (AHCI Enable)
        *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC) = ghc;
        
        fk::algorithms::klog("AHCI", "Controller disabled and destroyed");
    }
}

fk::core::Result<void, fk::core::Error> AHCIController::initialize_controller() {
    // Map PCI BAR5 (AHCI HBA memory space)
    uint32_t bar5 = PciManager::the().read_config_dword(m_pci_device.address(), 0x24);
    if ((bar5 & 0x01) == 0) { // Memory mapped
        uint32_t hba_phys_addr = bar5 & ~0x3Fu; // Clear bits 0-5
        
        // Map the HBA registers
        MemoryManager::the().map_page(hba_phys_addr, hba_phys_addr, 
                                     PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
        
        m_hba_base = reinterpret_cast<volatile uint8_t*>(hba_phys_addr);
        fk::algorithms::klog("AHCI", "HBA registers mapped at %p", (void*)(uintptr_t)hba_phys_addr);
    } else {
        fk::algorithms::kerror("AHCI", "IO space BAR not supported");
        return fk::core::Error::InvalidParameter;
    }
    
    // Read AHCI capabilities
    m_capabilities = *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_CAP);
    m_version = *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_VS);
    
    fk::algorithms::klog("AHCI", "AHCI version %d.%d, capabilities: 0x%08x", 
                         (m_version >> 16) & 0xFFFF, m_version & 0xFFFF, m_capabilities);
    
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
    
    // Configure interrupts
    configure_interrupts();
    
    // Scan for devices
    scan_ports();

    // Allocate memory for each port's Command List and FIS
    for (auto& port : m_ports) {
        if (!port.is_implemented) continue;

        // Allocate physical pages
        uintptr_t clb_phys = PhysicalMemoryManager::the().alloc_page();
        uintptr_t fb_phys = PhysicalMemoryManager::the().alloc_page();
        
        ASSERT(clb_phys != 0 && fb_phys != 0);

        // Identity map for simplicity (or should use VMM properly)
        MemoryManager::the().map_page(clb_phys, clb_phys, PageFlags::Present | PageFlags::Writable);
        MemoryManager::the().map_page(fb_phys, fb_phys, PageFlags::Present | PageFlags::Writable);

        port.regs->clb = (uint32_t)clb_phys;
        port.regs->clbu = (uint32_t)(clb_phys >> 32);
        port.regs->fb = (uint32_t)fb_phys;
        port.regs->fbu = (uint32_t)(fb_phys >> 32);

        // Clear memory
        memset(reinterpret_cast<void*>(clb_phys), 0, 4096);
        memset(reinterpret_cast<void*>(fb_phys), 0, 4096);

        // Enable port
        port.regs->cmd |= (1 << 4); // FRE (FIS Receive Enable)
        port.regs->cmd |= (1 << 0); // ST (Start)
    }
    
    m_initialized = true;
    return {};
}

fk::core::Result<void, fk::core::Error> AHCIController::reset_controller() {
    fk::algorithms::klog("AHCI", "Resetting controller...");
    
    // Enable AHCI mode
    uint32_t ghc = *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC);
    ghc |= (1u << 31); // AE (AHCI Enable)
    *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC) = ghc;
    
    // Reset controller
    ghc |= (1u << 0); // HR (Host Controller Reset)
    *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC) = ghc;
    
    // Wait for reset to complete
    int timeout = 1000;
    while (timeout-- > 0) {
        ghc = *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC);
        if ((ghc & (1u << 0)) == 0) {
            fk::algorithms::klog("AHCI", "Controller reset completed");
            return {};
        }
    }
    
    fk::algorithms::kerror("AHCI", "Controller reset timeout");
    return fk::core::Error::DeviceError;
}

void AHCIController::configure_interrupts() {
    // Disable all interrupts initially
    *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_IS) = 0xFFFFFFFF;
    
    // Enable global interrupt enable
    uint32_t ghc = *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC);
    ghc |= (1u << 1); // GIE (Global Interrupt Enable)
    *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_GHC) = ghc;
    
    fk::algorithms::klog("AHCI", "Interrupts configured");
}

void AHCIController::scan_ports() {
    uint32_t ports_implemented = *reinterpret_cast<volatile uint32_t*>(m_hba_base + HBA_PI);
    
    fk::algorithms::klog("AHCI", "Scanning ports, implemented mask: 0x%08x", ports_implemented);
    
    for (uint32_t i = 0; i < 32; ++i) {
        if ((ports_implemented & (1u << i)) == 0) {
            continue; // Port not implemented
        }
        
        Port port;
        port.index = i;
        port.is_implemented = true;
        port.has_device = false;
        port.sig = 0;
        port.regs = reinterpret_cast<volatile HBA_PORT*>(m_hba_base + 0x100 + (i * 0x80));
        
        // Check if port has device connected
        uint32_t ssts = port.regs->ssts; // Serial ATA Status
        
        // Check device detection (DET field)
        uint8_t det = ssts & 0x0F;
        if (det == 0x01 || det == 0x03) { // Device present
            port.has_device = true;
            port.sig = port.regs->sig; // Signature
            
            const char* device_type = "Unknown";
            switch (port.sig) {
                case 0x00000101: device_type = "ATA"; break;
                case 0xEB140101: device_type = "ATAPI"; break;
                case 0xC33C0101: device_type = "SEMB"; break;
                case 0x51424D45: device_type = "PM"; break; // Port Multiplier
            }
            
            fk::algorithms::klog("AHCI", "Port %u: %s device detected (signature: 0x%08x)", 
                                 i, device_type, port.sig);
        }
        
        m_ports.push_back(port);
    }
}

fk::core::Result<void, fk::core::Error> AHCIController::read_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, uint16_t* buffer) {
    if (port_idx >= m_ports.size()) return fk::core::Error::InvalidParameter;
    // AHCI Read logic here
    (void)start_sector; (void)count; (void)buffer;
    return fk::core::Error::NotImplemented;
}

fk::core::Result<void, fk::core::Error> AHCIController::write_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, const uint16_t* buffer) {
    if (port_idx >= m_ports.size()) return fk::core::Error::InvalidParameter;
    // AHCI Write logic here
    (void)start_sector; (void)count; (void)buffer;
    return fk::core::Error::NotImplemented;
}

void AHCIController::probe() {
    fk::algorithms::klog("AHCI", "Probing AHCI controller...");
    
    if (!m_initialized) {
        fk::algorithms::kwarn("AHCI", "Controller not initialized, skipping probe");
        return;
    }
    
    // TODO: Initialize ports with connected devices
    // This would involve setting up command lists, FIS receive areas, etc.
    
    fk::algorithms::klog("AHCI", "Probe completed");
}

fk::core::Result<size_t, fk::core::Error> 
AHCIController::read([[maybe_unused]] uint64_t offset, [[maybe_unused]] size_t size, [[maybe_unused]] uint8_t* buffer) {
    // TODO: Implement AHCI read operations
    // For now, return not implemented
    return fk::core::Error::NotImplemented;
}

fk::core::Result<size_t, fk::core::Error> 
AHCIController::write([[maybe_unused]] uint64_t offset, [[maybe_unused]] size_t size, [[maybe_unused]] const uint8_t* buffer) {
    // TODO: Implement AHCI write operations  
    // For now, return not implemented
    return fk::core::Error::NotImplemented;
}

size_t AHCIController::size() const {
    // TODO: Calculate total size of all attached devices
    return 0;
}

} // namespace fkernel