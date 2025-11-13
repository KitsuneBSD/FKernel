#include <Kernel/Block/Partition/GptPartition.h> 
#include <LibFK/Algorithms/log.h>

int GptPartitionStrategy::parse(const void* sector512, PartitionEntry* out, int max_out) {
  (void)sector512;
  (void)out;
  (void)max_out;
  
  kwarn("GPT", "GPT partition parsing not yet implemented");
  return 0;
}
