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
  // TODO: Improve ownership and lifecycle of boot resources using RAII
  // FIXME: Currently we assume serial and VGA init cannot fail; add error handling
  serial::init();
  auto vga = vga::the();
  vga.clear();
  // FIXME: multiboot2_magic is read as a 32-bit value from boot registers.
  // Ensure this matches all supported bootloaders; otherwise provide a
  // fallback path. Consider using a safer validation routine and returning
  // an error code instead of an assert in production builds.
  ASSERT(multiboot2_magic == multiboot2::BOOTLOADER_MAGIC);

  multiboot2::MultibootParser mb_parser(multiboot_ptr);
  auto mmap = mb_parser.find_tag<multiboot2::TagMemoryMap>(multiboot2::TagType::MMap);

  // Prefer framebuffer if provided by the bootloader (multiboot2).
  if (auto fb_tag = mb_parser.find_tag<multiboot2::TagFramebuffer>(multiboot2::TagType::Framebuffer)) {
    // Try to initialize VGA from framebuffer tag; fall back to text mode silently.
    if (!vga.initialize_framebuffer(fb_tag)) {
      // initialization failed; continue with text mode already set up
    }
  }

  early_init(mmap);

  // TODO: Replace the infinite HLT loop with a scheduler idle task or
  //       a proper kernel halt that can power-down or enter a more
  //       debuggable state. Also consider wiring a panic handler here.
  while (true) {
    asm("hlt");
  }
}

// UEFI-friendly entrypoint. Some UEFI bootloaders may call into the
// kernel directly after switching to long mode and provide an EFI memory
// map. This symbol lets external boot code call into the kernel using a
// simple C ABI: a pointer to the EFI memory map and the number of
// entries. The adapter will convert the EFI-style map into the internal
// MemoryMapView and continue initialization.
extern "C" void kmain_uefi(void const *efi_mmap, size_t entry_count)
{
  // Initialize basic output so early_init_from_uefi can use logging.
  serial::init();
  auto vga = vga::the();
  vga.clear();

  early_init_from_uefi(efi_mmap, entry_count);

  while (true) {
    asm("hlt");
  }
}
