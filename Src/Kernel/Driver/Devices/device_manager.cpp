#include <Kernel/Driver/Devices/console_device.h>
#include <Kernel/Driver/Devices/device_manager.h>
#include <Kernel/Driver/Devices/null_device.h>
#include <Kernel/Driver/Devices/serial_device.h>
#include <Kernel/Driver/Devices/zero_device.h>

#include <Kernel/Driver/Ata/AtaController.h>

#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>
#include <Kernel/FileSystem/file_descriptor.h>
#include <LibFK/Algorithms/log.h>

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

  // Open /dev/console for stdin, stdout, stderr
  int console_fd = file_descriptor_open_path("/dev/console", 0);
  if (console_fd >= 0) {
    // Duplicate the file descriptor to stdin (0), stdout (1), and stderr (2)
    FileDescriptorTable::the().dup2(console_fd, 0);
    FileDescriptorTable::the().dup2(console_fd, 1);
    FileDescriptorTable::the().dup2(console_fd, 2);
    file_descriptor_close(console_fd);
  } else {
    kwarn("DeviceManager", "Failed to open /dev/console");
  }

  // /dev/ada
  AtaController::the().initialize();
}
