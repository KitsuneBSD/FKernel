#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Text/string.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

display &display::the() {
  // Try framebuffer first (works for both UEFI and Multiboot2 with framebuffer)
  if (boot::BootInfo::the().has_framebuffer()) {
    return display_framebuffer::the();
  }

  // Fallback to VGA text mode
  return display_text::the();
}
