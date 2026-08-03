#pragma once 

#include <LibFK/Types/types.h>

struct MadtEntry {
    uint8_t type;
    uint8_t length;
}__attribute__((packed));