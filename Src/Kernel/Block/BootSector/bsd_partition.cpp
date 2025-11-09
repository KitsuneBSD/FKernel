#include <Kernel/Block/BootSector/bsd_partition.h>
#include <LibFK/Algorithms/log.h>

int parse_bsd_label(const AtaDeviceInfo &device, const void *sector512, PartitionEntry *out, int max_out) {
  (void) device;
  (void) sector512;
  (void) out;
  (void) max_out;

  kwarn("BSD DISK LABEL","We need implement this");
  return 0;
}