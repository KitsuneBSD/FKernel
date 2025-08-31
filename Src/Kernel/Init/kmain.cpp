#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>

#include <Kernel/Boot/early_init.h>

extern char __heap_start[];
extern char __heap_end[];

extern "C" void kmain(uint32_t multiboot2_magic, void *multiboot_ptr) {
  if (multiboot2_magic != multiboot2::BOOTLOADER_MAGIC) {
    while (true) {
      __asm__("hlt");
    }
  }

  multiboot2::MultibootParser mb_parser(multiboot_ptr);

  while (true) {
    asm("hlt");
  }
}


