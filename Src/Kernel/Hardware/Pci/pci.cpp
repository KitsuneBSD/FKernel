#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Acpi/mcfg.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>

void PciManager::initialize() {
  detect_legacy_ports();

  auto *mcfg = static_cast<MCFGTable *>(ACPIManager::the().find_table("MCFG"));
  if (mcfg) {
    size_t entries_count = (mcfg->header.length - sizeof(MCFGTable)) / sizeof(MCFGEntry);
    if (entries_count > 0) {
      m_mcfg_base = mcfg->entries[0].base_address;
      m_has_mcfg = true;
      fk::algorithms::klog("PCI", "MCFG found at %p, covering buses %d-%d", 
                           (void*)m_mcfg_base, mcfg->entries[0].start_bus_number, 
                           mcfg->entries[0].end_bus_number);
      
      // Map first few buses for scanning (e.g., 32MB covers 32 buses)
      // In a real kernel, we should map more or use a dynamic mapper
      for (uintptr_t addr = m_mcfg_base; addr < m_mcfg_base + (32 * 1024 * 1024); addr += 4096) {
          MemoryManager::the().map_page(addr, addr, PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);
      }
    }
  }

  if (!m_has_mcfg) {
    if (m_legacy_ports.functional) {
      fk::algorithms::klog("PCI", "MCFG not found, using legacy IO ports (0x%x/0x%x)", 
                           m_legacy_ports.address_port, m_legacy_ports.data_port);
    } else {
      fk::algorithms::klog("PCI", "Warning: MCFG not found and legacy ports don't seem functional!");
    }
  }

  fk::algorithms::klog("PCI", "Manager initialized");
}

void PciManager::detect_legacy_ports() {
  // Try to read VendorID of Bus 0, Device 0, Function 0
  uint32_t address = 0x80000000; // Bus 0, Dev 0, Func 0, Reg 0
  outl(m_legacy_ports.address_port, address);
  uint32_t value = inl(m_legacy_ports.data_port);

  if (value != 0xFFFFFFFF && value != 0x00000000) {
    m_legacy_ports.functional = true;
    fk::algorithms::klog("PCI", "Legacy ports 0x%x/0x%x are functional (B0:D0:F0 VendorID: 0x%x)", 
                         m_legacy_ports.address_port, m_legacy_ports.data_port, value & 0xFFFF);
  } else {
    m_legacy_ports.functional = false;
  }
}

PciManager &PciManager::the() {
  static PciManager s_instance;
  return s_instance;
}

uint32_t PciManager::read_config_dword(PciAddress address, uint8_t offset) {
  if (m_has_mcfg) {
    uintptr_t config_addr = m_mcfg_base + 
                            ((address.bus() << 20) | 
                             (address.device() << 15) | 
                             (address.function() << 12) | 
                             (offset & 0xFFF));
    return *reinterpret_cast<volatile uint32_t *>(config_addr);
  }

  outl(m_legacy_ports.address_port, address.to_config_address(offset));
  return inl(m_legacy_ports.data_port);
}

void PciManager::write_config_dword(PciAddress address, uint8_t offset, uint32_t value) {
  if (m_has_mcfg) {
    uintptr_t config_addr = m_mcfg_base + 
                            ((address.bus() << 20) | 
                             (address.device() << 15) | 
                             (address.function() << 12) | 
                             (offset & 0xFFF));
    *reinterpret_cast<volatile uint32_t *>(config_addr) = value;
  } else {
    outl(m_legacy_ports.address_port, address.to_config_address(offset));
    outl(m_legacy_ports.data_port, value);
  }
}

void PciManager::register_driver(uint8_t class_code, uint8_t subclass, DriverFactory factory) {
  fk::algorithms::klog("PCI", "Registering driver for Class %02x, Subclass %02x", class_code, subclass);
  m_drivers.push_back({class_code, subclass, fk::types::move(factory)});
}

void PciManager::instantiate_drivers() {
  fk::algorithms::klog("PCI", "Instantiating drivers for %d detected devices...", m_devices.size());
  for (const auto &device : m_devices) {
    bool driver_found = false;
    for (const auto &driver : m_drivers) {
      if (device.class_code() == driver.class_code && 
          device.subclass_code() == driver.subclass) {
        fk::algorithms::klog("PCI", "  -> Found driver for %02x:%02x.%d (Vendor:%04x Device:%04x)",
                             device.address().bus(), device.address().device(), 
                             device.address().function(), device.vendor_id(), device.device_id());
        driver.factory(device);
        driver_found = true;
      }
    }
    if (!driver_found) {
        // Optional: log devices without drivers at debug level
        // fk::algorithms::kdebug("PCI", "  No driver for %02x:%02x.%d (Class:%02x Sub:%02x)", ...);
    }
  }
}

void PciManager::auto_discover() {
  fk::algorithms::klog("PCI", "Starting auto-discovery...");
  scan_bus();
  instantiate_drivers();
  fk::algorithms::klog("PCI", "Auto-discovery complete. %d devices found.", m_devices.size());
}

void PciManager::scan_bus() {
  fk::algorithms::klog("PCI", "Starting PCI bus scan...");
  for (uint16_t bus = 0; bus < 256; ++bus) {
    for (uint8_t device = 0; device < 32; ++device) {
      check_device(bus, device);
    }
  }
  fk::algorithms::klog("PCI", "Scan complete. Found %d devices.",
                       m_devices.size());
}

void PciManager::check_device(uint8_t bus, uint8_t device) {
  uint8_t function = 0;
  PciAddress address(bus, device, function);
  uint32_t vendor_device = read_config_dword(address, 0);
  uint16_t vendor = vendor_device & 0xFFFF;

  if (vendor == 0xFFFF)
    return;

  check_function(bus, device, function);

  uint32_t header_type_reg = read_config_dword(address, 0x0C);
  if ((header_type_reg >> 16) & 0x80) {
    for (function = 1; function < 8; ++function) {
      PciAddress func_addr(bus, device, function);
      if ((read_config_dword(func_addr, 0) & 0xFFFF) != 0xFFFF) {
        check_function(bus, device, function);
      }
    }
  }
}

void PciManager::check_function(uint8_t bus, uint8_t device, uint8_t function) {
  PciAddress address(bus, device, function);
  uint32_t vendor_device = read_config_dword(address, 0);
  uint32_t class_reg = read_config_dword(address, 0x08);

  uint16_t vendor = vendor_device & 0xFFFF;
  uint16_t dev_id = vendor_device >> 16;
  uint8_t class_code = class_reg >> 24;
  uint8_t subclass = (class_reg >> 16) & 0xFF;
  uint8_t prog_if = (class_reg >> 8) & 0xFF;

  m_devices.push_back(
      PciDevice(address, vendor, dev_id, class_code, subclass, prog_if));

  fk::algorithms::klog("PCI",
                       "Found device %02x:%02x.%d - Vendor: %04x, Device: "
                       "%04x, Class: %02x, Sub: %02x, ProgIF: %02x",
                       bus, device, function, vendor, dev_id, class_code,
                       subclass, prog_if);
}
