#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/types.h>

enum class ChannelType : LibC::uint8_t {
    Primary = 0,
    Secondary = 1,
};

enum class DriveType : LibC::uint8_t {
    Master,
    Slave,
};

struct AtaDeviceInfo {
    ChannelType channel;
    DriveType drive;
    bool present;
};

struct ATAChannel {
    LibC::uint16_t io_base;
    LibC::uint16_t control_base;
    LibC::uint8_t irq;
};
