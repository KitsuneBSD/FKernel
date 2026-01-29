#include <Kernel/Boot/Uefi/uefi_loader.h>
#include <Kernel/Boot/Uefi/uefi_types.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Core/Assertions.h>
#include <LibC/string.h>

namespace uefi {

// ACPI 2.0 Table GUID
static const EFI_GUID ACPI_20_TABLE_GUID = {
  0x8868e871, 0xe4b0, 0x49b9,
  {0xe6, 0xe5, 0x27, 0x23, 0x7a, 0x51, 0x00, 0x00}
};

EFI_STATUS collect_memory_map(EFI_BOOT_SERVICES *boot_services,
                                size_t &memory_map_size,
                                EFI_MEMORY_DESCRIPTOR *&memory_map,
                                size_t &map_key,
                                size_t &descriptor_size,
                                uint32_t &descriptor_version) {
  if (!boot_services || !boot_services->GetMemoryMap) {
    return EFI_INVALID_PARAMETER;
  }

  // First call to get required size
  memory_map_size = 0;
  memory_map = nullptr;
  descriptor_size = 0;
  descriptor_version = 0;
  
  EFI_STATUS status = (*boot_services->GetMemoryMap)(
      &memory_map_size,
      memory_map,
      &map_key,
      &descriptor_size,
      &descriptor_version);

  // Allocate buffer for memory map
  // Note: We need to allocate more than the reported size to account for
  // descriptors that may be added during allocation
  // Use larger buffer for real hardware compatibility
  memory_map_size += 10 * descriptor_size;
  
  status = (*boot_services->AllocatePool)(
      static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiLoaderData),
      memory_map_size,
      reinterpret_cast<void **>(&memory_map));

  if (status != EFI_SUCCESS) {
    return status;
  }

  // Get the actual memory map
  status = (*boot_services->GetMemoryMap)(
      &memory_map_size,
      memory_map,
      &map_key,
      &descriptor_size,
      &descriptor_version);

  return status;
}

void extract_acpi_tables(EFI_SYSTEM_TABLE *system_table, boot::AcpiTableInfo &acpi_info) {
  if (!system_table || !system_table->ConfigurationTable) {
    return;
  }

  // Search for ACPI 2.0 table
  EFI_CONFIGURATION_TABLE *config_table = reinterpret_cast<EFI_CONFIGURATION_TABLE *>(system_table->ConfigurationTable);
  for (size_t i = 0; i < system_table->NumberOfTableEntries; ++i) {
    EFI_CONFIGURATION_TABLE *table = &config_table[i];
    
    // Compare GUIDs - compare all 16 bytes
    if (memcmp(&table->VendorGuid, &ACPI_20_TABLE_GUID, sizeof(EFI_GUID)) == 0) {
      // Found ACPI 2.0 table (XSDT)
      acpi_info.xsdt = table->VendorTable;
      
      // XSDT contains pointer to RSDP
      // RSDP is typically at the beginning of the XSDT or can be found via search
      // For now, we'll set rsdp to the XSDT pointer
      acpi_info.rsdp = table->VendorTable;
      break;
    }
  }
}

// Simple File System Protocol GUID
const EFI_GUID EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID = {
  0x0964e5b22, 0x6459, 0x11d2,
  {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

bool load_kernel_uefi_file(EFI_SYSTEM_TABLE *system_table, 
                          const uint16_t *filename,
                          uint64_t &kernel_entry) {
  if (!system_table || !system_table->BootServices || !filename) {
    return false;
  }

  EFI_BOOT_SERVICES *boot_services = system_table->BootServices;
  
  // Find all Simple File System protocol handles
  size_t num_handles = 0;
  EFI_HANDLE *handles = nullptr;
  
  EFI_STATUS status = (*boot_services->LocateHandleBuffer)(
      EFI_LOCATE_SEARCH_TYPE::ByProtocol,
      &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
      nullptr,
      &num_handles,
      &handles);
      
  if (status != EFI_SUCCESS || num_handles == 0) {
    return false;
  }

  // Try each handle to find the kernel file
  for (size_t i = 0; i < num_handles; ++i) {
    EFI_SIMPLE_FILE_SYSTEM_PROTOCOL *sfs = nullptr;
    status = (*boot_services->HandleProtocol)(
        handles[i],
        &EFI_SIMPLE_FILE_SYSTEM_PROTOCOL_GUID,
        reinterpret_cast<void **>(&sfs));
        
    if (status != EFI_SUCCESS) {
      continue;
    }

    // Open the volume
    EFI_FILE_HANDLE root = nullptr;
    status = (*sfs->OpenVolume)(sfs, &root);
    if (status != EFI_SUCCESS || !root) {
      continue;
    }

    // Try to open the kernel file
    EFI_FILE_HANDLE file = nullptr;
    status = (*root->Open)(
        root,
        &file,
        filename,
        0x01,  // EFI_FILE_MODE_READ
        0);     // No attributes
        
    if (status == EFI_SUCCESS && file) {
      // Get file size
      size_t buffer_size = 0;
      status = (*file->Read)(file, &buffer_size, nullptr);
      if (status == EFI_SUCCESS && buffer_size > 0) {
        // Allocate buffer
        void *buffer = nullptr;
        status = (*boot_services->AllocatePool)(
            static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiLoaderData),
            buffer_size,
            &buffer);
            
        if (status == EFI_SUCCESS && buffer) {
          // Read file content
          status = (*file->Read)(file, &buffer_size, buffer);
          if (status == EFI_SUCCESS) {
            // For now, assume this is the kernel entry point
            // TODO: Parse ELF to find actual entry point
            kernel_entry = reinterpret_cast<uint64_t>(buffer);
            
            (*file->Close)(file);
            (*root->Close)(root);
            return true;
          }
        }
      }
      (*file->Close)(file);
    }
    (*root->Close)(root);
  }

  return false;
}

} // namespace uefi
