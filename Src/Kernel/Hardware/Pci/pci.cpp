#include <Kernel/Arch/x86_64/io.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <LibFK/Algorithms/log.h>

static constexpr uint16_t PCI_CONFIG_ADDRESS = 0xCF8;
static constexpr uint16_t PCI_CONFIG_DATA = 0xCFC;

void PciManager::initialize() {
  fk::algorithms::klog("PCI", "Manager initialized");
}

PciManager &PciManager::the() {
  static PciManager s_instance;
  return s_instance;
}

uint32_t PciManager::read_config_dword(PciAddress address, uint8_t offset) {
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
