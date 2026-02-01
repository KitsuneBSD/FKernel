#pragma once

#include <Kernel/Hardware/Pci/pci_address.h>
#include <Kernel/Hardware/Pci/pci_device.h>
#include <Kernel/Hardware/Pci/pci_node.h>
#include <Kernel/Fs/Vfs/node.h>
#include <LibFK/Container/vector.h>
#include <LibFK/Container/hash_map.h>
#include <LibFK/Functional/Function.h>
#include <LibFK/Memory/ref_ptr.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {
template <> struct DefaultHasher<PciAddress> {
  size_t operator()(const PciAddress &value) const {
    return static_cast<size_t>(value.id());
  }
};
}
}

using DriverFactory = fk::functional::Function<fk::RefPtr<Node>(const PciDevice &)>;
using HotplugCallback = fk::functional::Function<void(const PciDevice &, bool is_insertion)>;

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
  uint16_t read_config_word(PciAddress address, uint8_t offset);
  uint8_t read_config_byte(PciAddress address, uint8_t offset);

  void write_config_dword(PciAddress address, uint8_t offset, uint32_t value);
  void write_config_word(PciAddress address, uint8_t offset, uint16_t value);
  void write_config_byte(PciAddress address, uint8_t offset, uint8_t value);

  void register_driver(uint8_t class_code, uint8_t subclass, DriverFactory factory);
  void instantiate_drivers();
  
  /// @brief Combines scan_bus() and instantiate_drivers() for automatic discovery
  void auto_discover();

  bool has_functional_legacy_ports() const { return m_legacy_ports.functional; }

  void enable_hotplug_detection();
  void register_hotplug_callback(HotplugCallback callback);
  void scan_for_new_devices();

private:
  PciManager() = default;
  fk::containers::Vector<PciDevice> m_devices;
  fk::containers::Vector<PciDriverEntry> m_drivers;
  fk::containers::Vector<HotplugCallback> m_hotplug_callbacks;
  fk::containers::HashMap<PciAddress, fk::RefPtr<Node>> m_device_nodes;
  fk::RefPtr<fkernel::PCIDeviceNode> m_pci_node;

  uintptr_t m_mcfg_base{0};
  bool m_has_mcfg{false};
  bool m_hotplug_enabled{false};
  PciLegacyPorts m_legacy_ports;

  void detect_legacy_ports();
  void check_device(uint8_t bus, uint8_t device);
  void check_function(uint8_t bus, uint8_t device, uint8_t function);
  void handle_hotplug_event(uint8_t bus, uint8_t device, uint8_t function);
};
