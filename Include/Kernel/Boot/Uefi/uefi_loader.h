#pragma once

#include <Kernel/Boot/Uefi/uefi_types.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Types/types.h>
#include <LibFK/Core/Assertions.h>

namespace uefi {

/**
 * @brief UEFI Memory Map Iterator
 * Implements the unified MemoryMapIterator interface for EFI memory maps
 */
class UefiMemoryMapIterator : public boot::MemoryMapIterator {
private:
  EFI_MEMORY_DESCRIPTOR *m_descriptors;
  size_t m_descriptor_count;
  size_t m_descriptor_size;
  size_t m_current_index;

public:
  UefiMemoryMapIterator(EFI_MEMORY_DESCRIPTOR *descriptors,
                        size_t descriptor_count,
                        size_t descriptor_size)
    : m_descriptors(descriptors),
      m_descriptor_count(descriptor_count),
      m_descriptor_size(descriptor_size),
      m_current_index(0) {}
  
  bool has_next() const override {
    return m_current_index < m_descriptor_count;
  }

  boot::MemoryMapEntry next() override {
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

  void reset() override {
    m_current_index = 0;
  }
};

/**
 * @brief Collect EFI Memory Map
 * @param boot_services EFI Boot Services
 * @param memory_map_size Output: size of memory map
 * @param memory_map Output: pointer to memory map
 * @param map_key Output: memory map key
 * @param descriptor_size Output: size of each descriptor
 * @return EFI_SUCCESS on success
 */
EFI_STATUS collect_memory_map(EFI_BOOT_SERVICES *boot_services,
                              size_t &memory_map_size,
                              EFI_MEMORY_DESCRIPTOR *&memory_map,
                              size_t &map_key,
                              size_t &descriptor_size,
                              uint32_t &descriptor_version);

/**
 * @brief Extract ACPI tables from EFI Configuration Table
 * @param system_table EFI System Table
 * @param acpi_info Output: ACPI table information
 */
void extract_acpi_tables(EFI_SYSTEM_TABLE *system_table,
                         boot::AcpiTableInfo &acpi_info);

/**
 * @brief Load kernel file using UEFI Simple File System Protocol
 * @param system_table EFI System Table
 * @param filename Path to kernel file
 * @param kernel_entry Output: kernel entry point address
 * @return true on success, false on failure
 */
bool load_kernel_uefi_file(EFI_SYSTEM_TABLE *system_table, 
                          const uint16_t *filename,
                          uint64_t &kernel_entry);

} // namespace uefi
