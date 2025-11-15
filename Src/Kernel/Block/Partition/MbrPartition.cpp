#include <Kernel/Block/Partition/MbrPartition.h>
#include <LibFK/Algorithms/log.h>

int MbrPartitionStrategy::parse(const void *sector512, PartitionEntry *out,
                                int max_out) {
  if (!sector512 || !out || max_out <= 0) {
    kwarn("MBR", "Invalid arguments to parser");
    return 0;
  }

  const uint8_t *data = static_cast<const uint8_t *>(sector512);

  if (!(data[510] == 0x55 && data[511] == 0xAA)) {
    kwarn("MBR", "Invalid MBR signature");
    return 0;
  }

  const MbrEntry *entries = reinterpret_cast<const MbrEntry *>(data + 446);

  int out_count = 0;

  for (int i = 0; i < 4 && out_count < max_out; i++) {
    const auto &e = entries[i];

    if (e.partition_type == 0 || e.sectors_count == 0)
      continue;

    PartitionEntry &pe = out[out_count++];

    pe.type = e.partition_type;
    pe.lba_start = e.start_lba;
    pe.lba_count = e.sectors_count;
    pe.is_bootable = (e.boot_indicator == 0x80);
    pe.has_chs = false;
  }

  klog("MBR", "Mbr partition identified and parsed");

  return out_count;
}
