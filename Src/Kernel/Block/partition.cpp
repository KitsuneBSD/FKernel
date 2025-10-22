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
  (void)sector512;
  return false;
}


// BSD disklabel basic structures (NetBSD/FreeBSD style, simplified)
struct bsd_disklabel {
  uint32_t d_magic; /* 0x82564557 */
  uint16_t d_type;
  uint16_t d_subtype;
  char d_packname[16];
  uint32_t d_secsize;
  uint32_t d_nsectors;
  uint32_t d_ntracks;
  uint32_t d_ncylinders;
  uint32_t d_secpercyl;
  uint32_t d_secperunit;
  uint32_t d_sparespertrack;
  uint32_t d_sparespercyl;
  uint32_t d_acylinders;
  uint32_t d_rpm;
  uint32_t d_interleave;
  uint32_t d_trackskew;
  uint32_t d_cylskew;
  uint32_t d_headswitch;
  uint32_t d_trkseek;
  uint32_t d_flags;
  uint32_t d_drivedata[5];
  uint32_t d_spare[5];
  char d_boot0[64];
  char d_boot1[64];
  char d_boot2[64];
  uint32_t d_map[16]; /* pairs of (start, size) for partitions */
  uint32_t d_magic2;
};

int parse_bsd_label(const AtaDeviceInfo &device, const void *sector512, PartitionEntry *out, int max_out) {
  (void)device;
  if (!sector512 || !out || max_out <= 0)
    return 0;

  // Search sector for BSD disklabel magic (0x82564557) at any 4-byte aligned offset
  const uint8_t *buf = reinterpret_cast<const uint8_t *>(sector512);
  const uint32_t MAGIC = 0x82564557u;
  const int buf_len = 512;
  int found_offset = -1;

  for (int off = 0; off + (int)sizeof(uint32_t) <= buf_len; off += 4) {
    uint32_t v;
    memcpy(&v, buf + off, sizeof(v));
    if (v == MAGIC) {
      // basic sanity check: ensure there's room for header
      if (off + (int)sizeof(struct bsd_disklabel) <= buf_len) {
        found_offset = off;
        break;
      }
    }
  }

  if (found_offset < 0)
    return 0;

  const bsd_disklabel *lbl = reinterpret_cast<const bsd_disklabel *>(buf + found_offset);

  // Additional validation: check trailing magic if present
  if (lbl->d_magic != MAGIC || (lbl->d_magic2 != MAGIC && lbl->d_magic2 != 0))
    return 0;

  // Partition map: `d_map` contains 16 u32 entries in pairs: (offset, size)
  int found = 0;
  for (int i = 0; i < 16 && found < max_out; i += 2) {
    uint32_t start = lbl->d_map[i];
    uint32_t size = lbl->d_map[i + 1];
    if (start == 0 && size == 0)
      continue;
    PartitionEntry p;
    p.boot_flag = 0;
    memset(p.chs_first, 0, 3);
    p.type = 0xA5; // BSD partition type marker (legacy)
    memset(p.chs_last, 0, 3);
    p.lba_first = start;
    p.sectors_count = size;
    out[found++] = p;
  }

  return found;
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
