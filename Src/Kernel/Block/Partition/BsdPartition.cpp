#include <Kernel/Block/Partition/BsdPartition.h>
#include <LibFK/Algorithms/log.h>

int BsdPartitionStrategy::parse(const void *sector512, PartitionEntry *output_partitions,
                                int max_partitions) {
  fk::algorithms::klog("BSD", "Attempting to parse BSD partition table.");
  (void)sector512;
  (void)output_partitions;
  (void)max_partitions;

  fk::algorithms::klog("BSD", "BSD partition parsing is not yet implemented.");
  return 0;
}
