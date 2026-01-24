#pragma once

#include <Kernel/Boot/Uefi/uefi_types.h>
#include <LibFK/Types/types.h>

namespace uefi {

/**
 * @brief Read blocks from a block device
 * @param block_io Block I/O protocol instance
 * @param media_id Media ID
 * @param lba Logical Block Address to start reading from
 * @param buffer_size Size of buffer in bytes
 * @param buffer Buffer to read into
 * @return EFI_SUCCESS on success, error code otherwise
 */
EFI_STATUS read_blocks(EFI_BLOCK_IO_PROTOCOL *block_io,
                       uint32_t media_id,
                       EFI_LBA lba,
                       size_t buffer_size,
                       void *buffer);

/**
 * @brief Find the first block device with a file system
 * @param system_table EFI System Table
 * @return Block I/O protocol instance, or nullptr if not found
 */
EFI_BLOCK_IO_PROTOCOL *find_block_device(EFI_SYSTEM_TABLE *system_table);

} // namespace uefi
