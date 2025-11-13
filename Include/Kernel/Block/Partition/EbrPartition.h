#pragma once 

#include <Kernel/Block/Partition/PartitionParsingStrategy.h>

class EbrPartitionStrategy : public PartitionParsingStrategy {
public:
    int parse(const void* sector512, PartitionEntry* out, int max_out) override;
};