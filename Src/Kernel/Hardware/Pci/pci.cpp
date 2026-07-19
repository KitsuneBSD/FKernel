#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Acpi/mcfg.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
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

  // Initialize event delivery node
  auto pci_node_res = fk::make_ref<fkernel::PCIDeviceNode>();
  if (pci_node_res.is_error()) {
      fk::algorithms::klog("PCI", "Failed to allocate PCI device node!");
  } else {
      m_pci_node = pci_node_res.value();
      fkernel::DevFs::the().register_device(m_pci_node, "pci");
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

uint16_t PciManager::read_config_word(PciAddress address, uint8_t offset) {
    uint32_t val = read_config_dword(address, offset & ~0x3);
    return (uint16_t)((val >> ((offset & 2) * 8)) & 0xFFFF);
}

uint8_t PciManager::read_config_byte(PciAddress address, uint8_t offset) {
    uint32_t val = read_config_dword(address, offset & ~0x3);
    return (uint8_t)((val >> ((offset & 3) * 8)) & 0xFF);
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

void PciManager::write_config_word(PciAddress address, uint8_t offset, uint16_t value) {
    uint32_t val = read_config_dword(address, offset & ~0x3);
    uint32_t mask = 0xFFFF << ((offset & 2) * 8);
    val = (val & ~mask) | (static_cast<uint32_t>(value) << ((offset & 2) * 8));
    write_config_dword(address, offset & ~0x3, val);
}

void PciManager::write_config_byte(PciAddress address, uint8_t offset, uint8_t value) {
    uint32_t val = read_config_dword(address, offset & ~0x3);
    uint32_t mask = 0xFF << ((offset & 3) * 8);
    val = (val & ~mask) | (static_cast<uint32_t>(value) << ((offset & 3) * 8));
    write_config_dword(address, offset & ~0x3, val);
}

uint32_t PciDevice::read_config_dword(uint8_t offset) const { return PciManager::the().read_config_dword(m_address, offset); }
uint16_t PciDevice::read_config_word(uint8_t offset) const { return PciManager::the().read_config_word(m_address, offset); }
uint8_t PciDevice::read_config_byte(uint8_t offset) const { return PciManager::the().read_config_byte(m_address, offset); }

void PciDevice::write_config_dword(uint8_t offset, uint32_t value) const { PciManager::the().write_config_dword(m_address, offset, value); }
void PciDevice::write_config_word(uint8_t offset, uint16_t value) const { PciManager::the().write_config_word(m_address, offset, value); }
void PciDevice::write_config_byte(uint8_t offset, uint8_t value) const { PciManager::the().write_config_byte(m_address, offset, value); }

static constexpr uint8_t BAR_OFFSET = 0x10;

bool PciDevice::bar_is_io(uint8_t index) const {
  return (read_config_dword(BAR_OFFSET + index * 4) & 0x1) != 0;
}

bool PciDevice::bar_is_64bit(uint8_t index) const {
  uint32_t bar = read_config_dword(BAR_OFFSET + index * 4);
  return (bar & 0x1) == 0 && ((bar >> 1) & 0x3) == 0x2;
}

uintptr_t PciDevice::bar_base(uint8_t index) const {
  if (index > 5) return 0;
  uint32_t bar = read_config_dword(BAR_OFFSET + index * 4);
  if (bar & 0x1) {
    return static_cast<uintptr_t>(bar & ~0x3u);
  }
  if (((bar >> 1) & 0x3) == 0x2 && index < 5) {
    uint64_t lo = bar & ~0xFu;
    uint64_t hi = read_config_dword(BAR_OFFSET + (index + 1) * 4);
    return static_cast<uintptr_t>(lo | (hi << 32));
  }
  return static_cast<uintptr_t>(bar & ~0xFu);
}

size_t PciDevice::bar_size(uint8_t index) const {
  if (index > 5) return 0;
  uint8_t reg = BAR_OFFSET + index * 4;
  uint32_t orig = read_config_dword(reg);
  write_config_dword(reg, 0xFFFFFFFF);
  uint32_t sz = read_config_dword(reg);
  write_config_dword(reg, orig);
  if (orig & 0x1) {
    sz &= ~0x3u;
  } else {
    sz &= ~0xFu;
  }
  if (sz == 0) return 0;
  return static_cast<size_t>(~sz + 1);
}

uint8_t PciDevice::find_capability(uint8_t cap_id) const {
    uint16_t status = read_config_word(0x06);
    if (!(status & (1 << 4))) return 0; // No capabilities list

    uint8_t cap_ptr = read_config_byte(0x34);
    while (cap_ptr != 0) {
        uint8_t id = read_config_byte(cap_ptr);
        if (id == cap_id) return cap_ptr;
        cap_ptr = read_config_byte(cap_ptr + 1);
    }
    return 0;
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
        auto node = driver.factory(device);
        if (node) {
            m_device_nodes.insert(device.address(), node);
        }
        driver_found = true;
      }
    }
    if (!driver_found) {
        // Optional: log devices without drivers at debug level
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

void PciManager::enable_hotplug_detection() {
    m_hotplug_enabled = true;
    fk::algorithms::klog("PCI", "Hotplug detection enabled");
}

void PciManager::register_hotplug_callback(HotplugCallback callback) {
    m_hotplug_callbacks.push_back(fk::types::move(callback));
}

void PciManager::scan_for_new_devices() {
    if (!m_hotplug_enabled) return;
    
    fk::algorithms::klog("PCI", "Scanning for new devices...");
    for (uint16_t bus = 0; bus < 256; ++bus) {
        for (uint8_t device = 0; device < 32; ++device) {
            handle_hotplug_event(bus, device, 0);
            
            // Check if we should scan more functions
            PciAddress addr(bus, device, 0);
            uint32_t header_type_reg = read_config_dword(addr, 0x0C);
            if (header_type_reg != 0xFFFFFFFF && ((header_type_reg >> 16) & 0x80)) {
                for (uint8_t function = 1; function < 8; ++function) {
                    handle_hotplug_event(bus, device, function);
                }
            }
        }
    }
}

void PciManager::handle_hotplug_event(uint8_t bus, uint8_t device, uint8_t function) {
    PciAddress address(bus, device, function);
    uint32_t vendor_device = read_config_dword(address, 0);
    uint16_t vendor = vendor_device & 0xFFFF;

    // Check if device exists
    bool exists = (vendor != 0xFFFF);
    
    // Check if we already know about this device
    int known_index = -1;
    for (size_t i = 0; i < m_devices.size(); ++i) {
        if (m_devices[i].address() == address) {
            known_index = (int)i;
            break;
        }
    }

    if (exists && known_index == -1) {
        // New device discovered
        uint32_t class_reg = read_config_dword(address, 0x08);
        uint16_t dev_id = vendor_device >> 16;
        uint8_t class_code = class_reg >> 24;
        uint8_t subclass = (class_reg >> 16) & 0xFF;
        uint8_t prog_if = (class_reg >> 8) & 0xFF;

        PciDevice new_device(address, vendor, dev_id, class_code, subclass, prog_if);
        m_devices.push_back(new_device);

        fk::algorithms::klog("PCI", "Hotplug: New device inserted at %02x:%02x.%d", bus, device, function);
        
        // Notify userspace node
        if (m_pci_node) {
            m_pci_node->push_event({fkernel::PCIEvent::Type::Insertion, bus, device, function, vendor, dev_id});
        }

        // Notify callbacks
        for (auto& cb : m_hotplug_callbacks) {
            cb(new_device, true);
        }

        // Try to instantiate driver
        for (const auto &driver : m_drivers) {
            if (new_device.class_code() == driver.class_code && 
                new_device.subclass_code() == driver.subclass) {
                auto node = driver.factory(new_device);
                if (node) {
                    m_device_nodes.insert(address, node);
                }
            }
        }
    } else if (!exists && known_index != -1) {
        // Device removed
        PciDevice removed_device = m_devices[known_index];
        
        // Notify callbacks before removing from list
        for (auto& cb : m_hotplug_callbacks) {
            cb(removed_device, false);
        }

        fk::algorithms::klog("PCI", "Hotplug: Device removed from %02x:%02x.%d", bus, device, function);
        
        // Notify userspace node
        if (m_pci_node) {
            m_pci_node->push_event({fkernel::PCIEvent::Type::Removal, bus, device, function, removed_device.vendor_id(), removed_device.device_id()});
        }

        // Unregister from DriverManager if we have a node
        auto node_opt = m_device_nodes.get(address);
        if (node_opt.has_value()) {
            fkernel::DriverManager::the().unregister_device(node_opt.value());
            m_device_nodes.remove(address);
        }

        // Remove from known devices
        m_devices[known_index] = m_devices[m_devices.size() - 1];
        m_devices.pop_back();
    }
}
