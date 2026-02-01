#pragma once

#include <LibC/stdint.h>

namespace fkernel {

struct PCIEvent {
    enum class Type : uint8_t {
        Insertion = 1,
        Removal = 0
    };

    Type type;
    uint8_t bus;
    uint8_t device;
    uint8_t function;
    uint16_t vendor;
    uint16_t device_id;
};

} // namespace fkernel
