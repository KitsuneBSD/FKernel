#pragma once

#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <LibC/stdint.h>

namespace Tss {

struct Tss {
    LibC::uint32_t reserved0;
    LibC::uint64_t rsp0;
    LibC::uint64_t rsp1;
    LibC::uint64_t rsp2;
    LibC::uint64_t reserved1;
    LibC::uint64_t ist[IST_COUNT];
    LibC::uint64_t reserved2;
    LibC::uint16_t reserved3;
    LibC::uint16_t io_map_base;
};
}
