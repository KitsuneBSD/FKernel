#include <Kernel/Block/Partition/EbrPartition.h>
#include <Kernel/Block/Partition/MbrPartition.h>
#include <LibFK/Algorithms/log.h>

int EbrPartitionStrategy::parse(const void *sector512, PartitionEntry *output_partitions,
                                int max_partitions) {
  (void)sector512;
  (void)output_partitions;
  (void)max_partitions;
  fk::algorithms::klog("EBR", "EBR partition parsing is not yet implemented.");
  return 0;
}
