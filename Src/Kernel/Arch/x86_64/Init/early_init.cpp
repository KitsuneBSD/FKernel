#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/hardware_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/timer_interrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Hardware/Acpi/acpi.h>
#include <Kernel/Hardware/Cpu/cpu.h>

#include <Kernel/Boot/Stages/early_init.h>
#include <Kernel/Boot/Stages/init.h>
#include <Kernel/Boot/boot_info.h>

#include <Kernel/Memory/memory_manager.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Memory/heap_malloc.h>

void early_init() {
  assert(boot::BootInfo::the().is_initialized() &&
         "early_init: BootInfo not initialized!");

  fk::algorithms::klog("EARLY_INIT", "Start early init");
  GDTController::the().initialize();

  // Heap MUST be initialized before Interrupts and Memory Manager
  // because they use strings and dynamic allocation.
  MemoryManager::the().initialize_heap();

  // Now that heap is ready, finalize BootInfo iterators
  boot::BootInfo::the().create_iterators();

  fk::algorithms::klog("EARLY_INIT", "Initializing Interrupts...");
  InterruptController::the().initialize();

  fk::algorithms::klog("EARLY_INIT", "Initializing Memory Manager...");
  MemoryManager::the().initialize();

  fk::algorithms::klog("EARLY_INIT", "Initializing ACPI...");
  ACPIManager::the().initialize();

  CPU::the().initialize_features();

  fk::algorithms::klog("EARLY_INIT", "Calling init()...");
  init();
}
