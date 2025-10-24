#include <Kernel/Boot/early_init.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#include <LibC/assert.h>
#include <LibC/stdio.h>

extern char __heap_start[];
extern char __heap_end[];

// UEFI-friendly entrypoint. Some UEFI bootloaders may call into the
// kernel directly after switching to long mode and provide an EFI memory
// map. This symbol lets external boot code call into the kernel using a
// simple C ABI: a pointer to the EFI memory map and the number of
// entries. The adapter will convert the EFI-style map into the internal
// MemoryMapView and continue initialization.
extern "C" void kmain_uefi(void const *efi_mmap, size_t entry_count)
{
  serial::init();
  auto vga = vga::the();
  vga.clear();

  early_init_from_uefi(efi_mmap, entry_count);

  while (true) {
    asm("hlt");
  }
}
