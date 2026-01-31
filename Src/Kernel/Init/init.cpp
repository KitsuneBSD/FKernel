#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Boot/Stages/init.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Driver/Storage/Ata/ata_controller.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Driver/driver_registry.h>
#include <Kernel/Fs/Vfs/virtual_filesystem.h>
#include <Kernel/Hardware/Pci/pci.h>
#include <Kernel/Scheduler/scheduler.h>
#include <Kernel/Syscall/syscall.h>

#include <Kernel/Driver/Terminal/terminal_manager.h>

void init() {
  // 0. Pre-init: Force serial logs for real hardware debugging
  fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial);
  fk::algorithms::klog("SYSTEM", "FKernel Live-Minimal starting on real hardware...");

  // Initialize PCI Manager (required for driver registry)
  PciManager::the().initialize();

  // Initialize VFS BEFORE storage detection to allow auto-mounting
  fkernel::VirtualFileSystem::initialize();

  // Auto-register all available drivers
  fkernel::DriverRegistry::initialize();

  // Auto-discover and instantiate drivers for detected devices
  PciManager::the().auto_discover();

  // Priorizar VESA sobre VGA
  if (boot::BootInfo::the().has_framebuffer()) {
    Display::switch_to(DisplayFramebuffer::the());
    
    // Enable on-screen logging now that display is ready
    fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial | fk::algorithms::LogTarget::Display);

    // Garantir que tty0 esteja ativo para userspace
    fkernel::terminal::TerminalManager::the().force_tty0_active();
    
    // Teste com mensagem visível
    Display::the().write("[SYSTEM] FKernel VESA + BusyBox rodando!\n");
    Display::the().flush();
    
    // Finalize display initialization after system is stable
    if (DisplayFramebuffer* fb_display = static_cast<DisplayFramebuffer*>(&Display::the())) {
      fb_display->finalize_initialization();
    }
  }

  // Initialize Driver Framework
  auto &driver_manager = fkernel::DriverManager::the();

  // Probe all registered drivers (handles any non-PCI drivers)
  driver_manager.probe_all();

  // 1. Inicializa TTYs primeiro
  fkernel::terminal::TerminalManager::the().initialize();

  // 2. Inicializa Teclado (legacy PS/2 device, not PCI)
  PS2Keyboard::the().initialize();
  driver_manager.register_device(fk::RefPtr<Node>(&PS2Keyboard::the()));

  SchedulerManager::the().initialize();
  SyscallManager::the().initialize();

  SchedulerManager::the().schedule();
}
