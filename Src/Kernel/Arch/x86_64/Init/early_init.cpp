#include <Kernel/Boot/early_init.h>
#include <LibC/stddef.h>
#include <LibC/stdio.h>
#include <LibFK/log.h>
#include <LibFK/type_traits.h>

#include <Kernel/Arch/x86_64/Interrupt/interrupt_controller.h>

void early_init([[maybe_unused]] const multiboot2::TagMemoryMap* mmap) {
  klog("MULTIBOOT2", "Reference to multiboot2 memory map: %p", mmap);

  for (auto entry : *mmap) {
    if (entry.type == 1) {
      kprintf("Memory map entry: addr=%p, len=%u, type=%u\n", entry.base_addr, entry.length, entry.type);  
    }
  } 

  InterruptController::the().initialize();
}
