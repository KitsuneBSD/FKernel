#pragma once

#include <Kernel/Block/Partition/PartitionParsingStrategy.h>

class BsdPartitionStrategy : public PartitionParsingStrategy {
public:
  int parse(const void *sector512, PartitionEntry *out, int max_out) override;
};