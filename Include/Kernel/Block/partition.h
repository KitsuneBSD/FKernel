#pragma once

#include <Kernel/Driver/Ata/AtaController.h>
#include <LibFK/Traits/types.h>

struct PartitionEntry {
  uint8_t boot_flag;
  uint8_t chs_first[3];
  uint8_t type;
  uint8_t chs_last[3];
  uint32_t lba_first;
  uint32_t sectors_count;
};

struct MBR {
  uint8_t bootstrap[446];
  PartitionEntry entries[4];
  uint16_t signature;
};

// Simple MBR parser: returns number of detected partitions (<=4)
int parse_mbr(const void *sector512, PartitionEntry out[4]);

// Parse EBR chain starting from the first EBR sector (returns number of
// logical partitions found, up to 'max_out'). Each out entry holds the
// partition entry from the EBR. Caller must provide a buffer for out.
int parse_ebr_chain(const AtaDeviceInfo &device, uint32_t ebr_lba_first, PartitionEntry *out, int max_out);

// Minimal GPT detection/parser: returns number of GPT partitions detected
// and fills `out` entries with LBA/size/type (type field set to 0xEE for GPT
// protective or the partition type's first byte). This is intentionally
// minimal: GUID decoding and attribute handling are omitted for now.
int parse_gpt(const AtaDeviceInfo &device, PartitionEntry *out, int max_out);

// Parse a BSD disklabel from a 512-byte sector buffer (typically LBA 1 or
// an alternate sector). This is a best-effort, conservative parser that
// scans the sector for the BSD disklabel magic (0x82564557) and attempts
// to extract partition (offset/size) pairs. On success fills up to
// `max_out` entries into `out` and returns the number of partitions found.
// Returns 0 on failure or when no valid BSD disklabel is detected.
int parse_bsd_label(const AtaDeviceInfo &device, const void *sector512, PartitionEntry *out, int max_out);
