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

  // Register drivers
  PciManager::the().register_driver(0x01, 0x01, [](const PciDevice &device) {
    ATAController::the().detect_on_pci(device);
  });

  PciManager::the().scan_bus();

  // Initialize VFS BEFORE storage detection to allow auto-mounting
  VirtualFileSystem::the().initialize();

  ATAController::the().initialize();
  
  // If no PCI IDE was found, fallback to legacy (handled by instantiate_drivers or manual call)
  PciManager::the().instantiate_drivers();
  
  // Optional: check if any devices were found, if not, try legacy
  if (ATAController::the().devices().is_empty()) {
      ATAController::the().detect_legacy();
  }

  PS2Keyboard::the().initialize();
  SchedulerManager::the().initialize();
  SyscallManager::the().initialize();

  SchedulerManager::the().schedule();
}
