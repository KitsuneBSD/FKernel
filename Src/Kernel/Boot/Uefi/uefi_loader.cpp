#include <Kernel/Boot/Uefi/uefi_loader.h>
#include <Kernel/Boot/Uefi/uefi_types.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Core/Assertions.h>

namespace uefi {

// ACPI 2.0 Table GUID
static const EFI_GUID ACPI_20_TABLE_GUID = {
  0x8868e871, 0xe4b0, 0x49b9,
  {0xe6, 0xe5, 0x27, 0x23, 0x7a, 0x51, 0x00, 0x00}
};

UefiMemoryMapIterator::UefiMemoryMapIterator(EFI_MEMORY_DESCRIPTOR *descriptors,
                                               size_t descriptor_count,
                                               size_t descriptor_size)
    : m_descriptors(descriptors),
      m_descriptor_count(descriptor_count),
      m_descriptor_size(descriptor_size),
      m_current_index(0) {
}

bool UefiMemoryMapIterator::has_next() const {
  return m_current_index < m_descriptor_count;
}

boot::MemoryMapEntry UefiMemoryMapIterator::next() {
  assert(has_next() && "UefiMemoryMapIterator: No more entries!");
  
  EFI_MEMORY_DESCRIPTOR *desc = reinterpret_cast<EFI_MEMORY_DESCRIPTOR *>(
      reinterpret_cast<uint8_t *>(m_descriptors) + (m_current_index * m_descriptor_size));
  
  boot::MemoryMapEntry entry;
  entry.base_addr = desc->PhysicalStart;
  entry.length = desc->NumberOfPages * 4096; // Convert pages to bytes
  entry.type = static_cast<uint32_t>(desc->Type);
  
  // Determine if memory is available
  entry.is_available = (desc->Type == static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiConventionalMemory) ||
                        desc->Type == static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiLoaderCode) ||
                        desc->Type == static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiLoaderData) ||
                        desc->Type == static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiBootServicesCode) ||
                        desc->Type == static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiBootServicesData) ||
                        desc->Type == static_cast<uint32_t>(EFI_MEMORY_TYPE::EfiACPIReclaimMemory));
  
  m_current_index++;
  return entry;
}

void UefiMemoryMapIterator::reset() {
  m_current_index = 0;
}

EFI_STATUS collect_memory_map(EFI_BOOT_SERVICES *boot_services,
                                size_t &memory_map_size,
                                EFI_MEMORY_DESCRIPTOR *&memory_map,
                                size_t &map_key,
                                size_t &descriptor_size) {
  if (!boot_services || !boot_services->GetMemoryMap) {
    return EFI_INVALID_PARAMETER;
  }

  // First call to get required size
  memory_map_size = 0;
  memory_map = nullptr;
  descriptor_size = 0;
  uint32_t descriptor_version = 0;
  
  EFI_STATUS status = (*boot_services->GetMemoryMap)(
      &memory_map_size,
      memory_map,
      &map_key,
      &descriptor_size,
      &descriptor_version);

  // Allocate buffer for memory map
  // Note: We need to allocate more than the reported size to account for
  // descriptors that may be added during allocation
  memory_map_size += 2 * descriptor_size;
  
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
    
    // Compare GUIDs
    if (table->VendorGuid.Data1 == ACPI_20_TABLE_GUID.Data1 &&
        table->VendorGuid.Data2 == ACPI_20_TABLE_GUID.Data2 &&
        table->VendorGuid.Data3 == ACPI_20_TABLE_GUID.Data3) {
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

} // namespace uefi
