#pragma once 

#include <Kernel/Block/partition.h>

int parse_bsd_label(const void *sector512, PartitionEntry *out, int max_out);
