#include <Kernel/Driver/Vga/display.h>
#include <Kernel/Driver/Vga/font.h>
#include <Kernel/Boot/multiboot2.h>
#include <LibFK/Text/string.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/io.h>
#endif

display &display::the() {
  if (boot::BootInfo::the().is_efi_boot()) {
    return display_efi::the();
  }

  return display_text::the();
}
