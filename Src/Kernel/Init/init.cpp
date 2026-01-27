#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Boot/Stages/init.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Driver/Storage/Ata/ata_controller.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

void init() {
  PciManager::the().initialize();
  PciManager::the().scan_bus();

  // Initialize VFS BEFORE storage detection to allow auto-mounting
  VirtualFileSystem::the().initialize();

  ATAController::the().initialize();
  ATAController::the().detect_devices();

  PS2Keyboard::the().initialize();
  SchedulerManager::the().initialize();
  SyscallManager::the().initialize();

  SchedulerManager::the().schedule();
}
