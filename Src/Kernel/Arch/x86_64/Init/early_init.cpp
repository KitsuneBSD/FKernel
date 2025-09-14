#include <Kernel/Arch/x86_64/Interrupt/idt.h>
#include <Kernel/Boot/early_init.h>
#include <LibC/stddef.h>
#include <LibC/stdio.h>
#include <LibFK/type_traits.h>

void early_init(const multiboot2::TagMemoryMap &mmap) {
  kprintf("Reference to multiboot2 memory map: %p\n", mmap);
  init_idt();
}
