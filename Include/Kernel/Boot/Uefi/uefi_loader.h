#pragma once

#include <Kernel/Boot/Uefi/uefi_types.h>
#include <Kernel/Boot/boot_info.h>
#include <LibFK/Types/types.h>

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
                        size_t descriptor_size);
  
  bool has_next() const override;
  boot::MemoryMapEntry next() override;
  void reset() override;
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
