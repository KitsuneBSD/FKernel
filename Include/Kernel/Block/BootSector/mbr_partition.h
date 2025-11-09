#pragma once 

#include <Kernel/Block/partition.h>

int parse_mbr(const void *sector512, PartitionEntry out[4]);
