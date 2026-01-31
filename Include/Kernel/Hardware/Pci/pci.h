#pragma once

#include <Kernel/Hardware/Pci/pci_address.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Functional/Function.h>
#include <LibFK/Types/types.h>

using DriverFactory = fk::functional::Function<void(const PciDevice &)>;

struct PciDriverEntry {
  uint8_t class_code;
  uint8_t subclass;
  DriverFactory factory;
};

struct PciLegacyPorts {
  uint16_t address_port{0xCF8};
  uint16_t data_port{0xCFC};
  bool functional{false};
};

class PciManager {
public:
  static PciManager &the();
  void initialize();

  void scan_bus();
  const fk::containers::Vector<PciDevice> &devices() const { return m_devices; }

  uint32_t read_config_dword(PciAddress address, uint8_t offset);
  void write_config_dword(PciAddress address, uint8_t offset, uint32_t value);

  void register_driver(uint8_t class_code, uint8_t subclass, DriverFactory factory);
  void instantiate_drivers();
  
  /// @brief Combines scan_bus() and instantiate_drivers() for automatic discovery
  void auto_discover();

  bool has_functional_legacy_ports() const { return m_legacy_ports.functional; }

private:
  PciManager() = default;
  fk::containers::Vector<PciDevice> m_devices;
  fk::containers::Vector<PciDriverEntry> m_drivers;

  uintptr_t m_mcfg_base{0};
  bool m_has_mcfg{false};
  PciLegacyPorts m_legacy_ports;

  void detect_legacy_ports();
  void check_device(uint8_t bus, uint8_t device);
  void check_function(uint8_t bus, uint8_t device, uint8_t function);
};
