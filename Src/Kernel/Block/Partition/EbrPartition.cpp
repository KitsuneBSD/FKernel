#include <Kernel/Block/Partition/EbrPartition.h>
#include <Kernel/Block/Partition/MbrPartition.h>
#include <LibFK/Algorithms/log.h>

int EbrPartitionStrategy::parse(const void *sector512, PartitionEntry *out,
                                int max_out) {
  (void)sector512;
  (void)out;
  (void)max_out;
  kwarn("EBR Partition", "We need implement this");
  return 0;
}