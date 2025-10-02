#include <Kernel/Arch/x86_64/Interrupt/HardwareInterrupts/pit.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Boot/early_init.h>
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>
#include <LibC/stddef.h>
#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Traits/type_traits.h>

#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>

void early_init([[maybe_unused]] const multiboot2::TagMemoryMap *mmap) {
  klog("MULTIBOOT2", "Reference to multiboot2 memory map: %p", mmap);

  GDTController::the().initialize();
  InterruptController::the().initialize();
  PhysicalMemoryManager::the().initialize(mmap);
  VirtualMemoryManager::the().initialize();

  asm volatile("int $0");
}
