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

  uint32_t read_config_dword(uint8_t offset) const;
  uint16_t read_config_word(uint8_t offset) const;
  uint8_t read_config_byte(uint8_t offset) const;

  void write_config_dword(uint8_t offset, uint32_t value) const;
  void write_config_word(uint8_t offset, uint16_t value) const;
  void write_config_byte(uint8_t offset, uint8_t value) const;

  uint8_t find_capability(uint8_t cap_id) const;

  // BAR accessors (BAR index 0–5)
  uintptr_t bar_base(uint8_t index) const;
  size_t    bar_size(uint8_t index) const;
  bool      bar_is_io(uint8_t index) const;
  bool      bar_is_64bit(uint8_t index) const;
};
