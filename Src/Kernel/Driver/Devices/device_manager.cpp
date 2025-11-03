#include <Kernel/Driver/Devices/console_device.h>
#include <Kernel/Driver/Devices/device_manager.h>
#include <Kernel/Driver/Devices/null_device.h>
#include <Kernel/Driver/Devices/serial_device.h>
#include <Kernel/Driver/Devices/zero_device.h>

#include <Kernel/Driver/Ata/AtaController.h>

#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/file_descriptor.h>

void init_basic_device() {
  // /Dev/null
  DevFS::the().register_device("null", VNodeType::CharacterDevice, &null_ops,
                               nullptr);

  // /Dev/zero
  DevFS::the().register_device("zero", VNodeType::CharacterDevice, &zero_ops,
                               nullptr);

  // /Dev/ttyS*
  DevFS::the().register_device("ttyS", VNodeType::CharacterDevice,
                               &SerialTTY::ops, nullptr, true);

  // /dev/console
  DevFS::the().register_device("console", VNodeType::CharacterDevice,
                               &ConsoleDevice::ops, nullptr);

  // /dev/ada
  AtaController::the().initialize();

  int fd = file_descriptor_open_path("/dev/console", 0);
  if (fd < 0)
    fd = file_descriptor_open_path("/dev/ttyS0", 0);

  if (fd >= 0) {
    (void)file_descriptor_open_path("/dev/console", 0);
    (void)file_descriptor_open_path("/dev/console", 0);
  }
}
