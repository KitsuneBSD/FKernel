#include <Kernel/Boot/early_init.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#include <LibC/assert.h>
#include <LibC/stdio.h>

extern char __heap_start[];
extern char __heap_end[];

extern "C" void kmain(uint32_t multiboot2_magic, void *multiboot_ptr) {
  serial::init();
  auto vga = vga::the();
  vga.clear();
  ASSERT(multiboot2_magic == multiboot2::BOOTLOADER_MAGIC);

  multiboot2::MultibootParser mb_parser(multiboot_ptr);

  auto mmap =
      mb_parser.find_tag<multiboot2::TagMemoryMap>(multiboot2::TagType::MMap);

  early_init(*mmap);

  while (true) {
    asm("hlt");
  }
}
