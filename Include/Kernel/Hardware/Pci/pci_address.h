#pragma once

#include <LibFK/Types/types.h>

class PciAddress {
  uint8_t m_bus;
  uint8_t m_device;
  uint8_t m_function;

public:
  PciAddress(uint8_t bus, uint8_t device, uint8_t function)
      : m_bus(bus), m_device(device), m_function(function) {}

  uint8_t bus() const { return m_bus; }
  uint8_t device() const { return m_device; }
  uint8_t function() const { return m_function; }

  uint32_t to_config_address(uint8_t offset) const {
    return (uint32_t)((m_bus << 16) | (m_device << 11) | (m_function << 8) |
                      (offset & 0xfc) | ((uint32_t)0x80000000));
  }
};
