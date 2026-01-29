#include <Kernel/Boot/Uefi/block_io.h>
#include <Kernel/Boot/Uefi/elf_loader.h>
#include <Kernel/Boot/Uefi/fat.h>
#include <Kernel/Boot/Uefi/gop.h>
#include <Kernel/Boot/Uefi/uefi_loader.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Boot/kernel_entry.h>
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <LibC/stdio.h>
#include <LibFK/Core/Assertions.h>

using namespace uefi;

extern "C" EFI_STATUS efi_main(EFI_HANDLE image_handle,
                               EFI_SYSTEM_TABLE *system_table) {
  // Initialize serial early for kprintf debugging
  serial::init();
  kprintf("UEFI efi_main started\n");

  // Get GOP framebuffer
  boot::FramebufferInfo framebuffer_info = uefi::initialize_gop(system_table);
  if (!framebuffer_info.addr) {
    // GOP not available or failed - will fall back to VGA text mode in kernel
  }

  // Note: We no longer load an external 'kernel.elf' because this binary 
  // already contains the full kernel code (Unified UEFI Kernel).

  // Collect EFI Memory Map
  size_t memory_map_size = 0;
  EFI_MEMORY_DESCRIPTOR *memory_map = nullptr;
  size_t map_key = 0;
  size_t descriptor_size = 0;
  uint32_t descriptor_version = 0;

  EFI_STATUS status =
      uefi::collect_memory_map(system_table->BootServices, memory_map_size,
                               memory_map, map_key, descriptor_size, descriptor_version);

  if (status != EFI_SUCCESS) {
    kprintf("ERROR: Failed to collect memory map!\n");
    return status;
  }

  // Calculate number of descriptors
  size_t descriptor_count = memory_map_size / descriptor_size;

  // Extract ACPI tables
  boot::AcpiTableInfo acpi_info{};
  uefi::extract_acpi_tables(system_table, acpi_info);

  // Initialize BootInfo
  boot::BootInfo::the().initialize_from_uefi(
      system_table, image_handle, framebuffer_info, memory_map,
      descriptor_count, descriptor_size, acpi_info);

  // Exit Boot Services
  // We must get a fresh memory map right before exiting to ensure map_key is valid
  status = uefi::collect_memory_map(system_table->BootServices, memory_map_size,
                                     memory_map, map_key, descriptor_size, descriptor_version);
  if (status != EFI_SUCCESS) {
    kprintf("ERROR: Failed to refresh memory map!\n");
    return status;
  }

  status = (*system_table->BootServices->ExitBootServices)(image_handle, map_key);
  if (status != EFI_SUCCESS) {
    kprintf("ERROR: Failed to exit boot services! (Map key might be outdated)\n");
    return status;
  }

  // Set up virtual address map for runtime services
  // This must be done AFTER ExitBootServices() according to UEFI spec
  status = (*system_table->RuntimeServices->SetVirtualAddressMap)(
      memory_map_size, descriptor_size, descriptor_version, memory_map);
  
  // Note: We can't easily kprintf here if SetVirtualAddressMap fails because
  // boot services are gone, but we can try to proceed to kernel_entry.

  // Mark boot services as no longer available
  boot::BootInfo::the().set_efi_boot_services_unavailable();

  // Call unified kernel entry point
  ::kernel_entry();

  // Should never reach here
  while (true) {
    asm volatile("hlt");
  }

  return EFI_SUCCESS;
}
