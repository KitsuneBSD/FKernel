#pragma once 

#include <Kernel/Block/partition.h>

int parse_ebr_chain(const AtaDeviceInfo &device, uint32_t ebr_lba_first, PartitionEntry *out, int max_out);