#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>
#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/apic.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>

#include <Kernel/Boot/multiboot2.h>

#include <Kernel/Hardware/Cpu.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

#include <LibFK/Algorithms/log.h>


void early_init([[maybe_unused]] const multiboot2::TagMemoryMap *mmap) {
  klog("MULTIBOOT2", "Reference to multiboot2 memory map: %p", mmap);

  GDTController::the().initialize();
  InterruptController::the().initialize();
  PhysicalMemoryManager::the().initialize(mmap);
  VirtualMemoryManager::the().initialize();

  if (CPU::the().has_apic()) {
    APIC::the().enable();
  }
}
