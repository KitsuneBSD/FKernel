#include <Kernel/Boot/early_init.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/Boot/boot_info.h>

#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#include <LibFK/Core/Assertions.h>
#include <LibC/stdio.h>

extern char __heap_start[];
extern char __heap_end[];

extern "C" void kmain(uint32_t multiboot2_magic, void *multiboot_ptr) {
  serial::init();

  auto &vga = vga::the();
  vga.clear();

  assert(multiboot2_magic == multiboot2::BOOTLOADER_MAGIC && "Invalid bootloader magic - kernel not loaded by compliant bootloader!");

  multiboot2::MultibootParser parser(multiboot_ptr);

  auto mmap_tag =
      parser.find_tag<multiboot2::TagMemoryMap>(multiboot2::TagType::MMap);
  assert(mmap_tag && "Memory map tag not found - cannot initialize memory!");

  auto fb_tag = parser.find_tag<multiboot2::TagFramebuffer>(
      multiboot2::TagType::Framebuffer);

  // Check if framebuffer tag was found - this is optional (graphics mode)
  if (!fb_tag) {
    kprintf("Framebuffer tag not found. Falling back to text mode.\n");
  }

  // Detect boot mode (BIOS vs EFI)
  boot::BootInfo::the().detect_boot_mode(multiboot_ptr);
  
  early_init(mmap_tag);

  while (true) {
    asm volatile("hlt");
  }
}