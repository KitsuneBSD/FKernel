#pragma once

#include <Kernel/Hardware/Pci/pci_address.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Types/types.h>

class PciManager {
public:
  static PciManager &the();
  void initialize();

  void scan_bus();
  const fk::containers::Vector<PciDevice> &devices() const { return m_devices; }

private:
  PciManager() = default;
  fk::containers::Vector<PciDevice> m_devices;

  uint32_t read_config_dword(PciAddress address, uint8_t offset);
  void check_device(uint8_t bus, uint8_t device);
  void check_function(uint8_t bus, uint8_t device, uint8_t function);
};
