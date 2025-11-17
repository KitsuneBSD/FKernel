#include <Kernel/Block/Partition/GptPartition.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/crc32.h>
#include <LibFK/Algorithms/log.h>

int GptPartitionStrategy::parse(const void *sector512, PartitionEntry *out,
                                int max_out) {
  if (!sector512 || !out || max_out <= 0) {
    kwarn("GPT", "Invalid parser arguments");
    return 0;
  }

  const uint8_t *base = static_cast<const uint8_t *>(sector512);

  const GptHeader *hdr = reinterpret_cast<const GptHeader *>(base);

  // --- GPT Header Size Check ---
  if (hdr->header_size < 92 || hdr->header_size > 512) {
    kwarn("GPT", "GPT Header size invalid: %u", hdr->header_size);
    return 0;
  }
  // --- End Header Size Check ---

  // --- GPT Header CRC32 Verification ---
  uint32_t original_header_crc32 = hdr->header_crc32;
  uint32_t header_size = hdr->header_size;

  uint8_t header_buffer[512];
  memcpy(header_buffer, base, header_size);

  // Zero out the CRC field for calculation. The offset of header_crc32 is 16.
  *reinterpret_cast<uint32_t *>(header_buffer + 16) = 0;

  // Calculate CRC32 of the header data.
  uint32_t calculated_header_crc32 = crc32(header_buffer, header_size);

  if (calculated_header_crc32 != original_header_crc32) {
    kwarn("GPT", "GPT Header CRC32 mismatch: expected %08x, got %08x",
          original_header_crc32, calculated_header_crc32);
    return 0; // CRC mismatch, invalid header
  }
  // --- End Header CRC32 Verification ---

  // Verifica assinatura "EFI PART"
  if (hdr->signature != 0x5452415020494645ULL) {
    kwarn("GPT", "Invalid signature");
    return 0;
  }

  if (hdr->partition_entry_size != sizeof(GptEntry)) {
    kwarn("GPT", "Unexpected GPT entry size: expected %zu, got %u",
          sizeof(GptEntry), hdr->partition_entry_size);
    return 0;
  }

  // --- Partition Entry Array CRC32 Verification ---
  // The partition entry array starts at LBA 2 (base + 512) in this
  // implementation, assuming sector512 is LBA 1. The PE CRC32 is stored in the
  // header.
  size_t num_entries = hdr->num_partition_entries;
  size_t entry_size = hdr->partition_entry_size;
  size_t partition_entry_array_size = num_entries * entry_size;

  // The partition entry array data starts at base + 512.
  const uint8_t *partition_entry_array_start = base + 512;

  // The PE CRC32 in the header is the CRC of the partition entry array.
  uint32_t expected_pe_crc32 = hdr->pe_crc32;

  // Calculate CRC32 of the partition entry array data.
  uint32_t calculated_pe_crc32 =
      crc32(partition_entry_array_start, partition_entry_array_size);

  if (calculated_pe_crc32 != expected_pe_crc32) {
    kwarn("GPT",
          "GPT Partition Entry Array CRC32 mismatch: expected %08x, got %08x",
          expected_pe_crc32, calculated_pe_crc32);
    return 0; // CRC mismatch, invalid partition entries
  }
  // --- End Partition Entry Array CRC32 Verification ---

  int count = hdr->num_partition_entries;
  if (count > max_out)
    count = max_out;

  int written = 0;
  for (int i = 0; i < count; i++) {
    const GptEntry &e = *reinterpret_cast<const GptEntry *>(
        partition_entry_array_start + i * entry_size);

    // Entry vazia = ignore

    bool empty = true;
    for (int j = 0; j < 16; j++) {
      if (e.type_guid[j] != 0) {
        empty = false;
        break;
      }
    }
    if (empty)
      continue;

    PartitionEntry &dst = out[written++];
    dst.lba_start = e.first_lba;
    dst.lba_count = (e.last_lba - e.first_lba) + 1;
    dst.type = 0;            // Tipo GPT não é representado em 8 bits
    dst.is_bootable = false; // GPT não usa flag de boot
    dst.has_chs = false;

    if (written >= max_out)
      break;
  }

  klog("GPT",
       "GPT partition identified and parsed successfully. Found %d partitions.",
       written);

  return written;
}
