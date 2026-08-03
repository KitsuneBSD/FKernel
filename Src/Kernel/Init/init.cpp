#include <Kernel/Arch/x86_64/Syscall/syscall_arch.h>
#include <Kernel/Boot/Stages/init.h>
#include <Kernel/Boot/Core/boot_info.h>
#include <Kernel/Boot/Core/boot_timer.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Driver/Mouse/ps2_mouse.h>
#include <Kernel/Driver/Storage/Controllers/Ata/ata_controller.h>
#include <Kernel/Driver/Storage/Partitions/partition_manager.h>
#include <Kernel/Driver/Vga/display_framebuffer.h>
#include <Kernel/Driver/Registry/driver_registry.h>
#include <Kernel/Fs/Vfs/Core/virtual_filesystem.h>
#include <Kernel/Hardware/Buses/Pci/pci.h>
#include <Kernel/Scheduler/Core/scheduler.h>
#include <Kernel/Syscall/syscall.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt_manager.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <LibFK/Algorithms/Crypto/chacha20.h>
#include <LibFK/Algorithms/Logging/log.h>

#include <Kernel/Driver/Terminal/terminal_manager.h>

static uint64_t rdtsc_entropy_source() {
  return arch_read_tsc();
}

void init() {
  BootTimer::the().mark("init_start");

  // Seed CSPRNG early — needed by ASLR, /dev/urandom, getrandom()
  fk::algorithms::ChaCha20PRNG::set_entropy_callback(rdtsc_entropy_source);
  fk::algorithms::ChaCha20PRNG::the().initialize();

  // Force serial-only logging until display is ready.
  fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial);
  fk::algorithms::klog("SYSTEM", "FKernel Live-Minimal starting on real hardware...");

  // Initialize PCI Manager (required for driver registry)
  fk::algorithms::klog("INIT", "Initializing PCI manager...");
  PciManager::the().initialize();
  if (!PciManager::the().is_initialized())
    fk::algorithms::kfatal("INIT", "PCI manager failed to initialize");
  BootTimer::the().mark("pci_init");

  // Initialize VFS BEFORE storage detection to allow auto-mounting
  fk::algorithms::klog("INIT", "Initializing VFS...");
  fkernel::VirtualFileSystem::initialize();
  BootTimer::the().mark("vfs_init");

  // Auto-register all available drivers
  fk::algorithms::klog("INIT", "Initializing driver registry...");
  fkernel::DriverRegistry::initialize();

  // Auto-discover and instantiate drivers for detected devices
  fk::algorithms::klog("INIT", "Auto-discovering PCI devices...");
  PciManager::the().auto_discover();
  BootTimer::the().mark("pci_discover");

  // Priorizar VESA sobre VGA
  if (boot::BootInfo::the().has_framebuffer()) {
    Display::switch_to(DisplayFramebuffer::the());

    // Enable on-screen logging now that display is ready
    fk::algorithms::set_log_targets(fk::algorithms::LogTarget::Serial);

    Display::the().flush();

    // Finalize display initialization after system is stable
    if (DisplayFramebuffer* fb_display = static_cast<DisplayFramebuffer*>(&Display::the())) {
      fb_display->finalize_initialization();
    }
  }

  // Initialize Driver Framework
  fk::algorithms::klog("INIT", "Initializing driver manager...");
  fkernel::DriverManager::the().initialize();
  if (!fkernel::DriverManager::the().is_initialized())
    fk::algorithms::kfatal("INIT", "Driver manager failed to initialize");

  fk::algorithms::klog("INIT", "Probing all drivers...");
  auto& driver_manager = fkernel::DriverManager::the();

  // Probe all registered drivers (handles any non-PCI drivers)
  driver_manager.probe_all();
  BootTimer::the().mark("drivers_probe");

  // 1. Inicializa TTYs primeiro
  fk::algorithms::klog("INIT", "Initializing terminal manager...");
  fkernel::terminal::TerminalManager::the().initialize();
  if (!fkernel::terminal::TerminalManager::the().is_initialized())
    fk::algorithms::kfatal("INIT", "Terminal manager failed to initialize");

  // Activate tty0 AFTER terminals exist
  fkernel::terminal::TerminalManager::the().force_tty0_active();

  // 2. Inicializa Teclado (legacy PS/2 device, not PCI)
  fk::algorithms::klog("INIT", "Initializing PS/2 keyboard...");
  PS2Keyboard::the().initialize();
  driver_manager.register_device(fk::RefPtr<Node>(&PS2Keyboard::the()));

  // 3. Register mouse device — hardware init deferred to first /dev/mouse open
  driver_manager.register_device(fk::RefPtr<Node>(&PS2Mouse::the()));

  // init_syscalls(0) must run before SchedulerManager::initialize(): once the scheduler
  // sets m_is_initialized=true any call to schedule()/load_next_task_context() reads
  // gs:32 for cpu_id. Without MSR_GS_BASE pointing to g_cpu_blocks[0] first that read
  // returns garbage from physical address 0x20 (BIOS IVT), producing a non-canonical
  // address that faults with GPF vector 13.
  fk::algorithms::klog("INIT", "Initializing syscall manager...");
  SyscallManager::the().initialize();
  if (!SyscallManager::the().is_initialized())
    fk::algorithms::kfatal("INIT", "Syscall manager failed to initialize");

  fk::algorithms::klog("INIT", "Initializing scheduler...");
  SchedulerManager::the().initialize();
  if (!SchedulerManager::the().is_initialized())
    fk::algorithms::kfatal("INIT", "Scheduler manager failed to initialize");
  BootTimer::the().mark("scheduler_init");

  fk::algorithms::klog("INIT", "Starting application processors...");
  SchedulerManager::the().start_aps();

  fk::algorithms::klog("INIT", "Enabling hardware interrupts...");
  HardwareInterruptManager::the().unmask_interrupt(0);  // Timer
  HardwareInterruptManager::the().unmask_interrupt(1);  // Keyboard
  HardwareInterruptManager::the().unmask_interrupt(8);  // Clock
  HardwareInterruptManager::the().unmask_interrupt(12); // PS/2 Mouse
  HardwareInterruptManager::the().unmask_interrupt(14); // Primary ATA
  HardwareInterruptManager::the().unmask_interrupt(15); // Secondary ATA
  InterruptController::the().enable_interrupt();

  // Disable interrupts for the scheduler transition. Timer interrupts firing
  // during boot log output and context initialization can race. The idle task's
  // idle_loop() will re-enable interrupts via sti;hlt.
  asm volatile("cli");

  fk::algorithms::klog("INIT", "Starting scheduler...");
  BootTimer::the().mark("init_end");
  BootTimer::the().log_summary();

  // The first schedule() needs need_resched=true to pick the idle task.
  // Interrupts are disabled (cli above), so no timer tick will set it.
  SchedulerManager::the().set_need_resched(true);
  SchedulerManager::the().schedule();
}
