#include <Kernel/Boot/Uefi/block_io.h>
#include <Kernel/Boot/Uefi/uefi_types.h>
#include <LibFK/Core/Assertions.h>

namespace uefi {

// Block I/O Protocol GUID
static const EFI_GUID EFI_BLOCK_IO_PROTOCOL_GUID = {
  0x964e5b22, 0x6459, 0x11d2,
  {0x8e, 0x39, 0x00, 0xa0, 0xc9, 0x69, 0x72, 0x3b}
};

EFI_STATUS read_blocks(EFI_BLOCK_IO_PROTOCOL *block_io,
                       uint32_t media_id,
                       EFI_LBA lba,
                       size_t buffer_size,
                       void *buffer) {
  if (!block_io || !block_io->ReadBlocks) {
    return EFI_INVALID_PARAMETER;
  }

  if (!block_io->Media || !block_io->Media->MediaPresent) {
    return EFI_NO_MEDIA;
  }

  size_t block_size = block_io->Media->BlockSize;
  size_t num_blocks = (buffer_size + block_size - 1) / block_size;

  typedef EFI_STATUS (*ReadBlocksFunc)(EFI_BLOCK_IO_PROTOCOL *, EFI_BLOCK_IO_MEDIA *, uint32_t, EFI_LBA, size_t, void *);
  ReadBlocksFunc read_func = reinterpret_cast<ReadBlocksFunc>(block_io->ReadBlocks);
  return read_func(block_io,
                   block_io->Media,
                   media_id,
                   lba,
                   num_blocks * block_size,
                   buffer);
}

EFI_BLOCK_IO_PROTOCOL *find_block_device(EFI_SYSTEM_TABLE *system_table) {
  if (!system_table || !system_table->BootServices) {
    return nullptr;
  }

  // Get all handles that support Block I/O Protocol
  EFI_HANDLE *handles = nullptr;
  size_t handle_count = 0;
  
  EFI_STATUS status = (*system_table->BootServices->LocateHandleBuffer)(
      static_cast<EFI_LOCATE_SEARCH_TYPE>(ByProtocol),
      &EFI_BLOCK_IO_PROTOCOL_GUID,
      nullptr,
      &handle_count,
      &handles);

  if (status != EFI_SUCCESS || handle_count == 0) {
    return nullptr;
  }

  // Try to find a device with a file system (check first few handles)
  for (size_t i = 0; i < handle_count && i < 16; ++i) {
    EFI_BLOCK_IO_PROTOCOL *block_io = nullptr;
    status = (*system_table->BootServices->HandleProtocol)(
        handles[i],
        &EFI_BLOCK_IO_PROTOCOL_GUID,
        reinterpret_cast<void **>(&block_io));

    if (status == EFI_SUCCESS && block_io && 
        block_io->Media && block_io->Media->MediaPresent) {
      // Found a valid block device
      return block_io;
    }
  }

  return nullptr;
}

} // namespace uefi
