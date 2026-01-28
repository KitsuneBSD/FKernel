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

  // Initialize VFS BEFORE storage detection to allow auto-mounting
  VirtualFileSystem::the().initialize();

  // Initialize Driver Framework
  auto& driver_manager = fkernel::DriverManager::the();
  
  // Register drivers
  PciManager::the().register_driver(0x01, 0x01, [](const PciDevice &device) {
    ATAController::the().detect_on_pci(device);
  });

  // Note: For now, ATAController is a singleton that we probe manually 
  // or via driver_manager if we wrapped it. 
  // Let's register it as a system driver.
  // Since we don't have a good way to 'new' it without losing the singleton ref, 
  // we'll just call its probe for now or implement a better wrapper.
  
  PciManager::the().scan_bus();

  // Trigger driver matching and device detection
  driver_manager.probe_all(); // If we had registered drivers there
  ATAController::the().detect_devices();

  PS2Keyboard::the().initialize();
  driver_manager.register_device(fk::RefPtr<Node>(&PS2Keyboard::the()));

  SchedulerManager::the().initialize();
  SyscallManager::the().initialize();

  SchedulerManager::the().schedule();
}
