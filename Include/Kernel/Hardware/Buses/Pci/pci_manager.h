#pragma once

#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Container/Associative/hash_map.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>
#include <Kernel/Hardware/Buses/Pci/pci_address.h>
#include <Kernel/Hardware/Buses/Pci/pci_device.h>
#include <Kernel/Hardware/Buses/Pci/pci_node.h>
#include <Kernel/Fs/Vfs/Core/node.h>
#include <LibFK/Functional/function.h>
#include <LibFK/Types/types.h>
#include <Kernel/Hardware/Buses/Pci/pci_driver_entry.h>
#include <Kernel/Hardware/Buses/Pci/pci_legacy_ports.h>

namespace fk {
namespace containers {
template <> struct DefaultHasher<PciAddress> {
  size_t operator()(const PciAddress &value) const {
    return static_cast<size_t>(value.id());
  }
};
}
}

namespace fkernel {

class PciManager {
  bool m_is_initialized{false};

  PciManager() = default;
  PciManager(const PciManager &) = delete;
  PciManager &operator=(const PciManager &) = delete;

public:
  static PciManager &the();
  bool is_initialized() const { return m_is_initialized; }
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

  void auto_discover();

  bool has_functional_legacy_ports() const { return m_legacy_ports.functional; }

  void enable_hotplug_detection();
  void register_hotplug_callback(HotplugCallback callback);
  void scan_for_new_devices();

  fk::containers::Vector<PciDevice> m_devices;
  fk::containers::Vector<PciDriverEntry> m_drivers;
  fk::containers::Vector<HotplugCallback> m_hotplug_callbacks;
  fk::containers::HashMap<PciAddress, fk::RefPtr<Node>> m_device_nodes;
  fk::RefPtr<PCIDeviceNode> m_pci_node;

  uintptr_t m_mcfg_base{0};
  uint8_t m_mcfg_start_bus{0};
  uint8_t m_mcfg_end_bus{255};
  bool m_has_mcfg{false};
  bool m_hotplug_enabled{false};
  PciLegacyPorts m_legacy_ports;

  void detect_legacy_ports();
  void check_device(uint8_t bus, uint8_t device);
  void check_function(uint8_t bus, uint8_t device, uint8_t function);
  void handle_hotplug_event(uint8_t bus, uint8_t device, uint8_t function);
};

} // namespace fkernel

using fkernel::PciManager;
