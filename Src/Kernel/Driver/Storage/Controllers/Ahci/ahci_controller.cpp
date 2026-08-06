#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <LibFK/Utilities/memory.h>

#include <Kernel/Driver/Storage/Controllers/Ahci/ahci_controller.h>
#include <Kernel/Memory/memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Hardware/Buses/Pci/pci.h>

namespace fkernel {

fk::RefPtr<AHCIController> AHCIController::create(const PciDevice& device) {
    fk::algorithms::klog("AHCI", "Creating controller for device %02x:%02x.%d (Vendor:%04x Device:%04x)",
                         device.address().bus(), device.address().device(), 
                         device.address().function(), device.vendor_id(), device.device_id());
    
    auto controller_result = fk::make_ref<AHCIController>(device);
    if (controller_result.is_error()) {
        fk::algorithms::kwarn("AHCI", "Failed to create controller instance");
        return nullptr;
    }
    
    auto controller = controller_result.value();
    auto init_result = controller->initialize_controller();
    if (init_result.is_error()) {
        fk::algorithms::kwarn("AHCI", "Failed to initialize controller: %d", (int)init_result.error());
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
        fk::algorithms::kwarn("AHCI", "IO space BAR not supported");
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

    // Allocate DMA buffers for each port's Command List, FIS, and Command Tables
    for (auto& port : m_ports) {
        if (!port.is_implemented) continue;

        if (port.cmd_list.allocate(1024).is_error() ||
            port.fis_buffer.allocate(256).is_error() ||
            port.cmd_tables.allocate(256 * 32).is_error()) {
            fk::algorithms::kwarn("AHCI", "Failed to allocate DMA memory for port");
            return fk::core::Error::OutOfMemory;
        }

        port.regs->clb = (uint32_t)port.cmd_list.physical_address().as_uintptr();
        port.regs->clbu = (uint32_t)(port.cmd_list.physical_address().as_uintptr() >> 32);
        port.regs->fb = (uint32_t)port.fis_buffer.physical_address().as_uintptr();
        port.regs->fbu = (uint32_t)(port.fis_buffer.physical_address().as_uintptr() >> 32);

        // Initialize Command Headers
        HBA_CMD_HEADER* cmd_headers = reinterpret_cast<HBA_CMD_HEADER*>(port.cmd_list.virtual_address());
        for (int i = 0; i < 32; i++) {
            uintptr_t ct_phys = port.cmd_tables.physical_address().as_uintptr() + (i * 256);
            cmd_headers[i].ctba = (uint32_t)ct_phys;
            cmd_headers[i].ctbau = (uint32_t)(ct_phys >> 32);
            cmd_headers[i].prdtl = 8; // 8 entries
        }

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
    
    fk::algorithms::kwarn("AHCI", "Controller reset timeout");
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
        
        AhciPort port;
        port.index = i;
        port.is_implemented = true;
        port.has_device = false;
        port.sig = 0;
        port.sectors = 0;
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
            
            if (port.sig == 0x00000101) {
                TRY_OR_FATAL(m_ports.push_back(port));
                uint32_t new_idx = m_ports.size() - 1;
                m_ports[new_idx].sectors = 1024 * 1024 * 2; // 1GB fallback
                auto sec_res = identify_port(new_idx);
                if (sec_res.is_ok() && sec_res.value() > 0)
                    m_ports[new_idx].sectors = sec_res.value();
                continue;
            }
        }
        
        TRY_OR_FATAL(m_ports.push_back(port));
    }
}

fk::core::Result<uint64_t, fk::core::Error> AHCIController::identify_port(uint32_t port_idx) {
    AhciPort& port = m_ports[port_idx];
    int slot = find_cmd_slot(port_idx);
    if (slot == -1) return fk::core::Error::DeviceError;

    static uint16_t identify_buf[256];
    fk::memory::set(identify_buf, 0, sizeof(identify_buf));

    HBA_CMD_HEADER* cmd_header = reinterpret_cast<HBA_CMD_HEADER*>(port.cmd_list.virtual_address());
    cmd_header += slot;
    cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmd_header->w = 0;
    cmd_header->prdtl = 1;

    HBA_CMD_TABLE* cmd_table = reinterpret_cast<HBA_CMD_TABLE*>(
        static_cast<uint8_t*>(port.cmd_tables.virtual_address()) + slot * 256);
    fk::memory::set(cmd_table, 0, sizeof(HBA_CMD_TABLE));

    cmd_table->prdt_entry[0].dba  = (uint32_t)VirtualMemoryManager::the().translate((uintptr_t)identify_buf);
    cmd_table->prdt_entry[0].dbau = (uint32_t)(VirtualMemoryManager::the().translate((uintptr_t)identify_buf) >> 32);
    cmd_table->prdt_entry[0].dbc  = 511; // 512 bytes - 1
    cmd_table->prdt_entry[0].i    = 1;

    FIS_REG_H2D* fis = (FIS_REG_H2D*)(&cmd_table->cfis);
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = ATA_CMD_IDENTIFY;
    fis->device = 0;

    int timeout = 1000000;
    while ((port.regs->tfd & (0x80 | 0x08)) && timeout > 0) --timeout;
    if (timeout == 0) return fk::core::Error::DeviceError;

    port.regs->is = 0xFFFFFFFF;
    port.regs->ci = (1 << slot);

    timeout = 1000000;
    while ((port.regs->ci & (1 << slot)) && timeout > 0) --timeout;
    if (timeout == 0) return fk::core::Error::DeviceError;

    if (port.regs->is & (1 << 30)) return fk::core::Error::DeviceError;

    uint64_t sectors = 0;
    if (identify_buf[83] & (1 << 10)) {
        sectors = (uint64_t)identify_buf[100]
                | ((uint64_t)identify_buf[101] << 16)
                | ((uint64_t)identify_buf[102] << 32)
                | ((uint64_t)identify_buf[103] << 48);
    }
    if (sectors == 0) {
        sectors = (uint64_t)identify_buf[60] | ((uint64_t)identify_buf[61] << 16);
    }
    fk::algorithms::klog("AHCI", "Port %u: %llu sectors (%llu MB)",
                          port_idx, sectors, sectors / 2048);
    return sectors;
}

int AHCIController::find_cmd_slot(uint32_t port_idx) {
    uint32_t slots = (m_ports[port_idx].regs->sact | m_ports[port_idx].regs->ci);
    for (int i = 0; i < 32; i++) {
        if ((slots & (1 << i)) == 0) return i;
    }
    return -1;
}

fk::core::Result<void, fk::core::Error> AHCIController::read_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, void* buffer) {
    if (port_idx >= m_ports.size()) {
        fk::algorithms::kwarn("AHCI", "read_port: invalid port %u", port_idx);
        return fk::core::Error::InvalidParameter;
    }
    AhciPort& port = m_ports[port_idx];

    port.regs->is = 0xFFFFFFFF; // Clear interrupts
    int slot = find_cmd_slot(port_idx);
    if (slot == -1) {
        fk::algorithms::kwarn("AHCI", "read_port: no command slot available");
        return fk::core::Error::DeviceError;
    }

    HBA_CMD_HEADER* cmd_header = reinterpret_cast<HBA_CMD_HEADER*>(port.cmd_list.virtual_address());
    cmd_header += slot;
    cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmd_header->w = 0; // Read
    cmd_header->prdtl = 1;

    HBA_CMD_TABLE* cmd_table = reinterpret_cast<HBA_CMD_TABLE*>(
        static_cast<uint8_t*>(port.cmd_tables.virtual_address()) + slot * 256);
    fk::memory::set(cmd_table, 0, sizeof(HBA_CMD_TABLE) + (cmd_header->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

    cmd_table->prdt_entry[0].dba = (uint32_t)VirtualMemoryManager::the().translate((uintptr_t)buffer);
    cmd_table->prdt_entry[0].dbau = (uint32_t)(VirtualMemoryManager::the().translate((uintptr_t)buffer) >> 32);
    cmd_table->prdt_entry[0].dbc = (count << 9) - 1; // 512 bytes per sector
    cmd_table->prdt_entry[0].i = 1;

    FIS_REG_H2D* fis = (FIS_REG_H2D*)(&cmd_table->cfis);
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = ATA_CMD_READ_DMA_EX;

    fis->lba0 = (uint8_t)start_sector;
    fis->lba1 = (uint8_t)(start_sector >> 8);
    fis->lba2 = (uint8_t)(start_sector >> 16);
    fis->device = 1 << 6; // LBA mode

    fis->lba3 = (uint8_t)(start_sector >> 24);
    fis->lba4 = (uint8_t)(start_sector >> 32);
    fis->lba5 = (uint8_t)(start_sector >> 40);

    fis->countl = (uint8_t)count;
    fis->counth = (uint8_t)(count >> 8);

    int timeout = 1000000;
    while ((port.regs->tfd & (0x80 | 0x08)) && timeout > 0) timeout--;
    if (timeout == 0) return fk::core::Error::DeviceError;

    port.regs->ci = (1 << slot);

    { int t = 5000000; while ((port.regs->ci & (1 << slot)) && t-- > 0) {
        if (port.regs->is & (1 << 30)) { fk::algorithms::kwarn("AHCI", "read_port: TFES error"); return fk::core::Error::DeviceError; }
    } if (t <= 0) { fk::algorithms::kwarn("AHCI", "read_port: CI timeout"); return fk::core::Error::DeviceError; } }

    if (port.regs->is & (1 << 30)) {
        fk::algorithms::kwarn("AHCI", "read_port: TFES error");
        return fk::core::Error::DeviceError;
    }

    return {};
}

fk::core::Result<void, fk::core::Error> AHCIController::write_port(uint32_t port_idx, uint64_t start_sector, uint32_t count, const void* buffer) {
    if (port_idx >= m_ports.size()) {
        fk::algorithms::kwarn("AHCI", "write_port: invalid port %u", port_idx);
        return fk::core::Error::InvalidParameter;
    }
    AhciPort& port = m_ports[port_idx];

    port.regs->is = 0xFFFFFFFF;
    int slot = find_cmd_slot(port_idx);
    if (slot == -1) {
        fk::algorithms::kwarn("AHCI", "write_port: no command slot available");
        return fk::core::Error::DeviceError;
    }

    HBA_CMD_HEADER* cmd_header = reinterpret_cast<HBA_CMD_HEADER*>(port.cmd_list.virtual_address());
    cmd_header += slot;
    cmd_header->cfl = sizeof(FIS_REG_H2D) / sizeof(uint32_t);
    cmd_header->w = 1; // Write
    cmd_header->prdtl = 1;

    HBA_CMD_TABLE* cmd_table = reinterpret_cast<HBA_CMD_TABLE*>(
        static_cast<uint8_t*>(port.cmd_tables.virtual_address()) + slot * 256);
    fk::memory::set(cmd_table, 0, sizeof(HBA_CMD_TABLE) + (cmd_header->prdtl - 1) * sizeof(HBA_PRDT_ENTRY));

    cmd_table->prdt_entry[0].dba = (uint32_t)VirtualMemoryManager::the().translate((uintptr_t)buffer);
    cmd_table->prdt_entry[0].dbau = (uint32_t)(VirtualMemoryManager::the().translate((uintptr_t)buffer) >> 32);
    cmd_table->prdt_entry[0].dbc = (count << 9) - 1;
    cmd_table->prdt_entry[0].i = 1;

    FIS_REG_H2D* fis = (FIS_REG_H2D*)(&cmd_table->cfis);
    fis->fis_type = FIS_TYPE_REG_H2D;
    fis->c = 1;
    fis->command = ATA_CMD_WRITE_DMA_EX;

    fis->lba0 = (uint8_t)start_sector;
    fis->lba1 = (uint8_t)(start_sector >> 8);
    fis->lba2 = (uint8_t)(start_sector >> 16);
    fis->device = 1 << 6;

    fis->lba3 = (uint8_t)(start_sector >> 24);
    fis->lba4 = (uint8_t)(start_sector >> 32);
    fis->lba5 = (uint8_t)(start_sector >> 40);

    fis->countl = (uint8_t)count;
    fis->counth = (uint8_t)(count >> 8);

    int timeout = 1000000;
    while ((port.regs->tfd & (0x80 | 0x08)) && timeout > 0) timeout--;
    if (timeout == 0) return fk::core::Error::DeviceError;

    port.regs->ci = (1 << slot);

    { int t = 5000000; while ((port.regs->ci & (1 << slot)) && t-- > 0) {
        if (port.regs->is & (1 << 30)) { fk::algorithms::kwarn("AHCI", "write_port: TFES error"); return fk::core::Error::DeviceError; }
    } if (t <= 0) { fk::algorithms::kwarn("AHCI", "write_port: CI timeout"); return fk::core::Error::DeviceError; } }

    if (port.regs->is & (1 << 30)) {
        fk::algorithms::kwarn("AHCI", "write_port: TFES error");
        return fk::core::Error::DeviceError;
    }

    return {};
}

void AHCIController::probe() {
    fk::algorithms::klog("AHCI", "Probing AHCI controller...");
    
    if (!m_initialized) {
        fk::algorithms::kwarn("AHCI", "Controller not initialized, skipping probe");
        return;
    }
    
    fk::algorithms::klog("AHCI", "Probe completed");
}

fk::core::Result<size_t, fk::core::Error> 
AHCIController::read_sectors(uint64_t start_sector, size_t count, uint8_t *buffer) {
    // For now, use port 0
    if (m_ports.is_empty()) return fk::core::Error::NotFound;
    auto res = read_port(0, start_sector, (uint32_t)count, buffer);
    if (res.is_error()) return res.error();
    return count;
}

fk::core::Result<size_t, fk::core::Error> 
AHCIController::write_sectors(uint64_t start_sector, size_t count, const uint8_t *buffer) {
    // For now, use port 0
    if (m_ports.is_empty()) return fk::core::Error::NotFound;
    auto res = write_port(0, start_sector, (uint32_t)count, buffer);
    if (res.is_error()) return res.error();
    return count;
}

SectorCount AHCIController::sector_count() const {
    if (m_ports.is_empty()) return SectorCount(0);
    return SectorCount(m_ports[0].sectors);
}

fk::core::Result<size_t, fk::core::Error> 
AHCIController::read(uint64_t offset, size_t size, uint8_t* buffer) {
    uint64_t start_sector = offset / 512;
    size_t count = (size + 511) / 512;
    // Note: buffer must be suitable for DMA (ideally physically contiguous)
    // For now, we assume it's okay (simplicity)
    return read_sectors(start_sector, count, buffer);
}

fk::core::Result<size_t, fk::core::Error> 
AHCIController::write(uint64_t offset, size_t size, const uint8_t* buffer) {
    uint64_t start_sector = offset / 512;
    size_t count = (size + 511) / 512;
    return write_sectors(start_sector, count, buffer);
}

size_t AHCIController::size() const {
    if (m_ports.is_empty()) return 0;
    return m_ports[0].sectors * 512;
}

} // namespace fkernel