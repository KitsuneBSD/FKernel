#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Acpi/mcfg.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>

static constexpr uint16_t PCI_CONFIG_ADDRESS = 0xCF8;
static constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

void PciManager::initialize() {
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
    fk::algorithms::klog("PCI", "MCFG not found, using legacy IO ports");
  }

  fk::algorithms::klog("PCI", "Manager initialized");
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

  outl(PCI_CONFIG_ADDRESS, address.to_config_address(offset));
  return inl(PCI_CONFIG_DATA);
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
