#include <Kernel/Boot/Stages/early_init.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <LibC/stdio.h>
#include <LibFK/Core/Assertions.h>

extern char __heap_start[];
extern char __heap_end[];

/**
 * @brief Unified kernel entry point
 * Works for both Multiboot2 and UEFI boot
 * BootInfo must be initialized before calling this
 */
extern "C" void kernel_entry() {
  serial::init();

  auto &vga = vga::the();
  vga.clear();

  assert(boot::BootInfo::the().is_initialized() && "BootInfo not initialized!");

  kprintf("FKernel starting...\n");
  kprintf("Boot mode: %s\n", boot::BootInfo::the().is_uefi_boot() ? "UEFI"
                             : boot::BootInfo::the().is_multiboot2_boot()
                                 ? "Multiboot2"
                                 : "Unknown");

  if (boot::BootInfo::the().has_framebuffer()) {
    auto fb = boot::BootInfo::the().get_framebuffer_info();
    kprintf("Framebuffer: %ux%u @ %p\n", fb.width, fb.height,
            reinterpret_cast<void *>(fb.addr));
  } else {
    kprintf("No framebuffer, using VGA text mode\n");
  }

  early_init();

  while (true) {
    asm volatile("hlt");
  }
}
