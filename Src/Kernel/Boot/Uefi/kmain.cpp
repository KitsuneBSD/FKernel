#include <Kernel/Boot/Uefi/block_io.h>
#include <Kernel/Boot/Uefi/elf_loader.h>
#include <Kernel/Boot/Uefi/fat.h>
#include <Kernel/Boot/Uefi/gop.h>
#include <Kernel/Boot/Uefi/uefi_loader.h>
#include <Kernel/Boot/boot_info.h>
#include <Kernel/Boot/kernel_entry.h>
#include <LibC/stdio.h>
#include <LibFK/Core/Assertions.h>

using namespace uefi;

extern "C" EFI_STATUS efi_main(EFI_HANDLE image_handle,
                               EFI_SYSTEM_TABLE *system_table) {
  // Note: Serial and VGA initialization will happen in kernel_entry
  // We can't use them here as we're still in UEFI boot services

  // Get GOP framebuffer
  boot::FramebufferInfo framebuffer_info = uefi::initialize_gop(system_table);
  if (!framebuffer_info.addr) {
    // GOP not available or failed - will fall back to VGA text mode in kernel
  }

  // Find block device
  EFI_BLOCK_IO_PROTOCOL *block_io = uefi::find_block_device(system_table);
  if (!block_io) {
    kprintf("ERROR: No block device found!\n");
    return EFI_NOT_FOUND;
  }

  // Initialize FAT filesystem
  uefi::fat::FatContext fat_context;
  if (!uefi::fat::initialize(block_io, fat_context)) {
    kprintf("ERROR: Failed to initialize FAT filesystem!\n");
    return EFI_LOAD_ERROR;
  }

  // Load kernel ELF
  uint64_t kernel_entry = 0;
  if (!uefi::load_kernel(fat_context, "kernel.elf", kernel_entry)) {
    kprintf("ERROR: Failed to load kernel!\n");
    return EFI_LOAD_ERROR;
  }

  // Collect EFI Memory Map
  size_t memory_map_size = 0;
  EFI_MEMORY_DESCRIPTOR *memory_map = nullptr;
  size_t map_key = 0;
  size_t descriptor_size = 0;

  EFI_STATUS status =
      uefi::collect_memory_map(system_table->BootServices, memory_map_size,
                               memory_map, map_key, descriptor_size);

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
  status =
      (*system_table->BootServices->ExitBootServices)(image_handle, map_key);
  if (status != EFI_SUCCESS) {
    kprintf("ERROR: Failed to exit boot services!\n");
    return status;
  }

  // Mark boot services as no longer available
  boot::BootInfo::the().set_efi_boot_services_unavailable();

  // Call unified kernel entry point
  // Note: We're already in long mode when UEFI calls us
  // Use fully qualified name to avoid conflict with local variable
  ::kernel_entry();

  // Should never reach here
  while (true) {
    asm volatile("hlt");
  }

  return EFI_SUCCESS;
}
