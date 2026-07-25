#include <Kernel/Driver/Device/driver_manager.h>
#include <Kernel/Fs/DevFs/dev_fs.h>
#include <LibFK/Algorithms/log.h>

namespace fkernel {

DriverManager& DriverManager::the() {
  static DriverManager instance;
  return instance;
}

void DriverManager::initialize() {
  if (m_is_initialized) return;
  fk::algorithms::klog("DRIVER_MANAGER", "DriverManager initialized");
  m_is_initialized = true;
}

fk::core::Result<void, fk::core::Error>
DriverManager::register_driver(fk::memory::OwnPtr<Driver> driver) {
  if (!driver) {
    fk::algorithms::kerror("DRIVER MANAGER", "register_driver: null driver pointer");
    return fk::core::Error::InvalidParameter;
  }

  fk::algorithms::klog("DRIVER MANAGER", "Registering new driver: '%s'", driver->name());
  m_drivers.push_back(fk::types::move(driver));
  return {};
}

fk::core::Result<void, fk::core::Error> DriverManager::register_device(fk::RefPtr<Node> device) {
  if (!device) {
    fk::algorithms::kerror("DRIVER MANAGER", "register_device: null device pointer");
    return fk::core::Error::InvalidParameter;
  }

  fk::algorithms::klog(
      "DRIVER MANAGER", "Registering device: '%s' (Type: %s)", device->name().c_str(),
      device->is_block_device() ? "Block" : (device->is_character_device() ? "Char" : "Unknown"));
  m_devices.push_back(device);

  // Automatically register with DevFs
  auto res = DevFs::the().register_device(device, device->name().c_str());
  if (res.is_error()) {
    fk::algorithms::kwarn("DRIVER MANAGER", "Failed to expose '%s' in DevFs: %d",
                          device->name().c_str(), (int)res.error());
  } else {
    fk::algorithms::klog("DRIVER MANAGER", "Device '%s' exposed in DevFs successfully",
                         device->name().c_str());
  }

  return {};
}

void DriverManager::unregister_device(fk::RefPtr<Node> device) {
  for (size_t i = 0; i < m_devices.size(); ++i) {
    if (m_devices[i] == device) {
      fk::algorithms::klog("DRIVER MANAGER", "Unregistering device: '%s'", device->name().c_str());
      DevFs::the().unregister_device(device->name().c_str());

      // Fast remove
      m_devices[i] = m_devices[m_devices.size() - 1];
      m_devices.pop_back();
      return;
    }
  }
  fk::algorithms::kwarn("DRIVER MANAGER", "unregister_device: device not found in registry");
}

void DriverManager::probe_all() {
  fk::algorithms::klog("DRIVER MANAGER", "Probing all %d registered drivers...", m_drivers.size());
  for (auto& driver : m_drivers) {
    fk::algorithms::klog("DRIVER MANAGER", "Probing driver: '%s'...", driver->name());
    driver->probe();
  }
  fk::algorithms::klog("DRIVER MANAGER", "Probe sequence completed.");
}

} // namespace fkernel
