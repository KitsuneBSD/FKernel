#include "Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockInterrupt.h"
#include <Kernel/Boot/init.h>
#include <LibFK/Algorithms/log.h>

#include <Kernel/Driver/Devices/device_manager.h>
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/RamFS/ramfs.h>
#include <Kernel/FileSystem/VirtualFS/vfs.h>

#include <Kernel/Driver/Ata/AtaController.h>

#include <Kernel/FileSystem/Fat/fat_fs.h>
#include <Kernel/Driver/Ata/AtaFs.h>
#include <Kernel/FileSystem/FilesystemManager.h>

void init() {
  fkernel::fs::fat::FatFileSystem::early_register();
  fkernel::drivers::ata::AtaFs::early_register();
  auto &vfs = VirtualFS::the();
  auto &ramfs = RamFS::the();
  auto &devfs = DevFS::the();
  fk::algorithms::klog("INIT", "Start init");

  vfs.mount("/", ramfs.root());

  fk::memory::RetainPtr<VNode> root;
  if (vfs.lookup("/", root) != 0) {
    fk::algorithms::kwarn("VFS", "Root lookup failed");
    return;
  }

  vfs.mount("dev", devfs.root());

  fk::memory::RetainPtr<VNode> dev;
  vfs.lookup("/dev", dev);

  init_basic_device();
  // Initialize ATA controller to detect physical drives
  AtaController::the().initialize();
  // Now initialize the FilesystemManager to probe and mount filesystems on detected partitions
  fkernel::fs::FilesystemManager::the().initialize();
  ClockManager::the().datetime().print();
}
