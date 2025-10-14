#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/Handler/handlers.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/init.h>

#include <Kernel/Hardware/Cpu.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

#include <LibFK/Algorithms/log.h>

void early_init([[maybe_unused]] const multiboot2::TagMemoryMap *mmap)
{
  klog("EARLY_INIT", "Start early init");

  GDTController::the().initialize();
  InterruptController::the().initialize();
  PhysicalMemoryManager::the().initialize(mmap);
  VirtualMemoryManager::the().initialize();

  if (CPU::the().has_apic())
  {
    APIC::the().enable();
    APIC::the().calibrate_timer();

    InterruptController::the().register_interrupt(apic_timer_handler, 32);
    APIC::the().setup_timer(1);
  }

  init();
}
