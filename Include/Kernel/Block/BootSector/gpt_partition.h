#pragma once 

#include <Kernel/Block/partition.h>

int parse_gpt(const AtaDeviceInfo &device, PartitionEntry *out, int max_out);
