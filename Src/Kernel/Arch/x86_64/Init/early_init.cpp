#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Hardware/Acpi/acpi.h>

#include <Kernel/Boot/early_init.h>
#include <Kernel/Boot/init.h>
#include <Kernel/Boot/multiboot2.h>

#include <Kernel/Memory/MemoryManager.h>
#include <LibFK/Core/Assertions.h>
#include <LibFK/Algorithms/log.h>

void early_init([[maybe_unused]] const multiboot2::TagMemoryMap *mmap) {
  assert(mmap && "early_init: Memory map not provided!");
  
  fk::algorithms::klog("EARLY_INIT", "Start early init (multiboot2)");

  GDTController::the().initialize();
  InterruptController::the().initialize();
  MemoryManager::the().initialize(mmap);

  ACPIManager::the().initialize();

  init();
}
