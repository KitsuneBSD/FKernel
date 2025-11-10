#pragma once 

#include <Kernel/Block/partition.h>

int parse_gpt(PartitionEntry *out, int max_out);
