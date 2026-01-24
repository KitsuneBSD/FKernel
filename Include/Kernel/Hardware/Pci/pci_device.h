#pragma once

#include <Kernel/Hardware/Pci/pci_address.h>
#include <LibFK/Types/types.h>

class PciDevice {
  PciAddress m_address;
  uint16_t m_vendor_id;
  uint16_t m_device_id;
  uint8_t m_class_code;
  uint8_t m_subclass_code;
  uint8_t m_prog_if;

public:
  PciDevice(PciAddress address, uint16_t vendor, uint16_t device,
            uint8_t class_code, uint8_t subclass, uint8_t prog_if)
      : m_address(address), m_vendor_id(vendor), m_device_id(device),
        m_class_code(class_code), m_subclass_code(subclass),
        m_prog_if(prog_if) {}

  PciAddress address() const { return m_address; }
  uint16_t vendor_id() const { return m_vendor_id; }
  uint16_t device_id() const { return m_device_id; }
  uint8_t class_code() const { return m_class_code; }
  uint8_t subclass_code() const { return m_subclass_code; }
  uint8_t prog_if() const { return m_prog_if; }
};
