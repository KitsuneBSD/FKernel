#include <Kernel/Block/BootSector/gpt_partition.h> 
#include <LibFK/Algorithms/log.h>

int parse_gpt(PartitionEntry *out, int max_out) {
  (void)out;
  (void)max_out;
  
  kwarn("GPT", "GPT partition parsing not yet implemented");
  return 0;
}
