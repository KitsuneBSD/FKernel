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

  auto &vga = vga::the();
  vga.clear();

  if (multiboot2_magic != multiboot2::BOOTLOADER_MAGIC) {
    kprintf("Invalid bootloader magic: 0x%x\n", multiboot2_magic);
  }

  multiboot2::MultibootParser parser(multiboot_ptr);

  auto mmap_tag =
      parser.find_tag<multiboot2::TagMemoryMap>(multiboot2::TagType::MMap);
  ASSERT(mmap_tag);

  auto fb_tag = parser.find_tag<multiboot2::TagFramebuffer>(
      multiboot2::TagType::Framebuffer);

  // Check if framebuffer tag was found
  if (!fb_tag) {
    kprintf("Framebuffer tag not found. Falling back to text mode.\n");
  } else {
    // Attempt to initialize framebuffer mode
    if (!vga.initialize_framebuffer(fb_tag)) {
      kprintf("Failed to initialize framebuffer. Falling back to text mode.\n");
    }
  }

  early_init(mmap_tag);

  while (true) {
    asm volatile("hlt");
  }
}