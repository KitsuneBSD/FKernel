#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/ClockController/rtc.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/HardwareInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/TimerInterrupt.h>
#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Hardware/acpi.h>

#include <Kernel/Boot/early_init.h>
#include <Kernel/Boot/multiboot2.h>

#include <Kernel/Hardware/cpu.h>
#include <Kernel/Memory/MemoryManager.h>

#include <Kernel/Memory/PhysicalMemoryManager.h>
#include <LibFK/Algorithms/log.h>

void early_init([[maybe_unused]] const multiboot2::TagMemoryMap *mmap) {
  fk::algorithms::klog("EARLY_INIT", "Start early init (multiboot2)");

  GDTController::the().initialize();
  InterruptController::the().initialize();
  MemoryManager::the().initialize(mmap);

  HardwareInterruptManager::the().initialize();
  TimerManager::the().initialize(100);
  ClockManager::the().initialize();

  //ACPIManager::the().initialize();
}
