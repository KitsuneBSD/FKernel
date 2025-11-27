#include <Kernel/Driver/Devices/serial_device.h>

// Define the VNodeOps for SerialTTY
VNodeOps SerialTTY::ops = {
    .read = &SerialTTY::read,
    .write = &SerialTTY::write,
    .open = &SerialTTY::open,
    .close = &SerialTTY::close,
    .lookup = nullptr,
    .create = nullptr,
    .readdir = nullptr,
    .unlink = nullptr,
};
