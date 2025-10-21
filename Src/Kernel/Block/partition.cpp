#include <Kernel/Block/partition.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Traits/types.h>
#include <LibC/string.h>

int parse_mbr(const void *sector512, PartitionEntry out[4]) {
  if (!sector512 || !out)
    return 0;

  auto *mbr = reinterpret_cast<const MBR *>(sector512);
  if (mbr->signature != 0xAA55)
    return 0;

  int count = 0;
  for (int i = 0; i < 4; ++i) {
    const auto &e = mbr->entries[i];
    // Basic validation: non-zero type and sectors_count
    if (e.type != 0 && e.sectors_count != 0) {
      out[count++] = e;
    }
  }

  return count;
}

bool parse_bsd_label(const void *sector512) {
  // TODO: Implement BSD disklabel parsing (e.g., NetBSD/FreeBSD formats).
  // For now, return false as a stub.
  (void)sector512;
  return false;
}

int parse_ebr_chain(const AtaDeviceInfo &device, uint32_t ebr_lba_first, PartitionEntry *out, int max_out) {
  if (!out || max_out <= 0)
    return 0;

  int found = 0;
  uint32_t current_ebr = ebr_lba_first;
  uint8_t sector[512];

  while (current_ebr != 0 && found < max_out) {
    if (AtaController::the().read_sectors_pio(device, current_ebr, 1, sector) <= 0)
      break;

    PartitionEntry entries[4];
    int pc = parse_mbr(sector, entries);
    // In EBR the first entry is the logical partition, the second points to next EBR
    if (pc > 0 && entries[0].type != 0 && entries[0].sectors_count != 0) {
      out[found++] = entries[0];
    }
    if (pc > 1 && entries[1].type != 0 && entries[1].sectors_count != 0) {
      // entries[1].lba_first is relative to the original base. To chain, we
      // take its lba_first and add current_ebr to get next EBR absolute LBA.
      current_ebr = entries[1].lba_first + current_ebr;
    } else {
      break;
    }
  }

  return found;
}

int parse_gpt(const AtaDeviceInfo &device, PartitionEntry *out, int max_out) {
  if (!out || max_out <= 0)
    return 0;

  uint8_t sector[512];
  // GPT header is at LBA 1
  if (AtaController::the().read_sectors_pio(device, 1, 1, sector) <= 0)
    return 0;

  // Check for 'EFI PART' signature at offset 0
  if (memcmp(sector, "EFI PART", 8) != 0)
    return 0;

  // Read partition entry LBA and count: at offsets 72 (partition entry LBA, little endian)
  uint64_t part_entry_lba = *reinterpret_cast<uint64_t *>(sector + 72);
  uint32_t num_entries = *reinterpret_cast<uint32_t *>(sector + 80);
  uint32_t size_of_entry = *reinterpret_cast<uint32_t *>(sector + 84);

  if (num_entries == 0 || size_of_entry == 0)
    return 0;

  int read_needed = (num_entries * size_of_entry + 511) / 512;
  int found = 0;
  // Read entries and fill simple PartitionEntry structures
  for (int i = 0; i < read_needed && found < max_out; ++i) {
    if (AtaController::the().read_sectors_pio(device, static_cast<uint32_t>(part_entry_lba + i), 1, sector) <= 0)
      break;
    for (int j = 0; j < 512 / (int)size_of_entry && found < max_out; ++j) {
      uint8_t *entry = sector + j * size_of_entry;
      // partition type GUID is 16 bytes at offset 0; if zero => unused
      bool empty = true;
      for (int k = 0; k < 16; ++k) if (entry[k] != 0) { empty = false; break; }
      if (empty) continue;

      // Starting LBA at offset 32, 8 bytes; size LBA at offset 40
      uint64_t start_lba = *reinterpret_cast<uint64_t *>(entry + 32);
      uint64_t len_lba = *reinterpret_cast<uint64_t *>(entry + 40);

      PartitionEntry p;
      p.boot_flag = 0;
      memset(p.chs_first, 0, 3);
      p.type = 0xEE; // indicate GPT-derived
      memset(p.chs_last, 0, 3);
      p.lba_first = static_cast<uint32_t>(start_lba);
      p.sectors_count = static_cast<uint32_t>(len_lba);
      out[found++] = p;
    }
  }

  return found;
}
