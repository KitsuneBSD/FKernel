#pragma once

#include <Kernel/Block/PartitionEntry.h>
#include <LibFK/Types/types.h>

class PartitionParsingStrategy {
public:
    virtual ~PartitionParsingStrategy() = default;
    virtual int parse(const void* sector512, PartitionEntry* out, int max_out) = 0;
};
