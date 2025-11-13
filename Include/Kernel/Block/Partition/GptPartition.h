#pragma once 

#include <Kernel/Block/Partition/PartitionParsingStrategy.h>

class GptPartitionStrategy : public PartitionParsingStrategy {
public:
    int parse(const void* sector512, PartitionEntry* out, int max_out) override;
};
