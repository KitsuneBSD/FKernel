#pragma once

#include <LibFK/Types/types.h>

struct PciLegacyPorts {
  uint16_t address_port{0xCF8};
  uint16_t data_port{0xCFC};
  bool functional{false};
};
