#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct Fat12DirectoryEntry {
    char name[8];
    char ext[3];
    uint8_t attr;
    uint8_t lcase;
    uint8_t creation_time_ms;
    uint16_t creation_time;
    uint16_t creation_date;
    uint16_t last_access_date;
    uint16_t cluster_high;
    uint16_t update_time;
    uint16_t update_date;
    uint16_t cluster_low;
    uint32_t size;
} __attribute__((packed));

}
