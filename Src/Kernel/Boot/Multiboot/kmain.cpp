#include <Kernel/Boot/Multiboot/multiboot2.h>
#include <Kernel/Boot/Multiboot/multiboot_interpreter.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Boot/kernel_entry.h>
#include <LibFK/Core/assertions.h>

// kmain is the very first C function called by the bootloader.
// Serial and logging are NOT yet initialized — only assert is safe here.
extern "C" void kmain(uint32_t multiboot2_magic, void *multiboot_ptr) {
  assert(
      multiboot2_magic == multiboot2::BOOTLOADER_MAGIC &&
      "Invalid bootloader magic - kernel not loaded by compliant bootloader!");

  // Parse multiboot2 tags into unified BootInfo.
  // Logging starts in kernel_entry() after serial::init().
  boot::BootInfo::the().initialize_from_multiboot2(multiboot_ptr);

  kernel_entry();
}
