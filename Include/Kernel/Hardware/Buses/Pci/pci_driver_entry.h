#pragma once

#include <LibFK/Functional/function.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>

using DriverFactory = fk::functional::Function<fk::RefPtr<Node>(const PciDevice &)>;
using HotplugCallback = fk::functional::Function<void(const PciDevice &, bool is_insertion)>;

struct PciDriverEntry {
  uint8_t class_code;
  uint8_t subclass;
  DriverFactory factory;
};
