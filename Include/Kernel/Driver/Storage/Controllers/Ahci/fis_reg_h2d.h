#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {

struct FIS_REG_H2D {
    uint8_t  fis_type;
    uint8_t  pmport:4;
    uint8_t  reserved0:3;
    uint8_t  c:1;
    uint8_t  command;
    uint8_t  featurel;
    uint8_t  lba0;
    uint8_t  lba1;
    uint8_t  lba2;
    uint8_t  device;
    uint8_t  lba3;
    uint8_t  lba4;
    uint8_t  lba5;
    uint8_t  featureh;
    uint8_t  countl;
    uint8_t  counth;
    uint8_t  icc;
    uint8_t  control;
    uint8_t  reserved1[4];
} __attribute__((packed));

} // namespace fkernel
