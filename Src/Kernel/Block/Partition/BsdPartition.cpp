#include <Kernel/Block/Partition/BsdPartition.h>
#include <LibFK/Algorithms/log.h>

int BsdPartitionStrategy::parse(const void *sector512, PartitionEntry *out,
                                int max_out) {
  (void)sector512;
  (void)out;
  (void)max_out;

  fk::algorithms::kwarn("BSD DISK LABEL", "We need implement this");
  return 0;
}
