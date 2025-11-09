#include <Kernel/Block/BootSector/mbr_partition.h>
#include <LibFK/Algorithms/log.h>

int parse_mbr(const void *sector512, PartitionEntry out[4]) {
  (void)sector512;
  (void)out;
  
  kwarn("MBR", "MBR partition parsing not yet implemented");
  return 0;
}
