#pragma once

#include <Kernel/Devices/Storage/BlockDeviceTypes.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace Device {

struct BlockDevice {
    char const* name;
    LibC::uint64_t block_size;
    LibC::uint64_t block_count;
    void* private_data;

    BlockDeviceOps* ops;
};

}
