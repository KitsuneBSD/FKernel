#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Boot/Stages/init.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Driver/Storage/Ahci/ahci_controller.h>
#include <Kernel/Driver/Storage/Ata/ata_controller.h>
#include <Kernel/Driver/Storage/Nvme/nvme_controller.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

#include <Kernel/Driver/Terminal/terminal_manager.h>

void init() {
  PciManager::the().initialize();

  // Initialize VFS BEFORE storage detection to allow auto-mounting
  VirtualFileSystem::the().initialize();

  // Ativar Framebuffer assim que o MemoryManager (e agora o VFS) estiver pronto
  if (boot::BootInfo::the().has_framebuffer()) {
    fk::algorithms::klog("INIT",
                         "Switching to native bootloader framebuffer...");
    Display::switch_to(DisplayFramebuffer::the());
  } else {
    fk::algorithms::klog("INIT",
                         "No native framebuffer, attempting VESA upgrade...");
    auto vesa_res = DisplayFramebuffer::the().set_resolution(1024, 768, 32);
    if (vesa_res.is_ok()) {
      Display::switch_to(DisplayFramebuffer::the());
    }
  }

  // Initialize Driver Framework
  auto &driver_manager = fkernel::DriverManager::the();

  // Register drivers
  PciManager::the().register_driver(0x01, 0x01, [](const PciDevice &device) {
    ATAController::the().detect_on_pci(device);
  });

  PciManager::the().register_driver(0x01, 0x06, [](const PciDevice &device) {
    auto controller = AHCIController::create(device);
    fkernel::DriverManager::the().register_device(controller);
  });

  PciManager::the().register_driver(0x01, 0x08, [](const PciDevice &device) {
    auto controller = NVMeController::create(device);
    fkernel::DriverManager::the().register_device(controller);
  });

  // Note: For now, ATAController is a singleton that we probe manually
  // or via driver_manager if we wrapped it.
  // Let's register it as a system driver.
  // Since we don't have a good way to 'new' it without losing the singleton
  // ref, we'll just call its probe for now or implement a better wrapper.

  PciManager::the().scan_bus();

  // Trigger driver matching and device detection
  driver_manager.probe_all(); // If we had registered drivers there
  ATAController::the().detect_devices();

  // 1. Inicializa TTYs primeiro
  fkernel::terminal::TerminalManager::the().initialize();

  // 2. Inicializa Teclado
  PS2Keyboard::the().initialize();
  driver_manager.register_device(fk::RefPtr<Node>(&PS2Keyboard::the()));

  SchedulerManager::the().initialize();
  SyscallManager::the().initialize();

  SchedulerManager::the().schedule();
}
