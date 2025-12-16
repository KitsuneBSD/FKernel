#pragma once 

#include <LibFK/Types/types.h>

struct PageTable{
    uint64_t entries[512]; // TODO: Use a custom type to Page integer
}__attribute__((aligned(4096)));