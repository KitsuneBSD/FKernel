#include <Kernel/Driver/Devices/console_device.h>
#include <Kernel/Driver/Keyboard/ps2_keyboard.h>
#include <Kernel/Driver/Vga/vga_buffer.h>

#include <LibFK/Algorithms/log.h>

int ConsoleDevice::open(VNode *vnode, int flags)
{
    (void)vnode;
    (void)flags;
    klog("CONSOLE", "Opened /dev/console");
    return 0;
}

int ConsoleDevice::close(VNode *vnode)
{
    (void)vnode;
    klog("CONSOLE", "Closed /dev/console");
    return 0;
}

int ConsoleDevice::write(VNode *vnode, const void *buffer, size_t size, size_t offset)
{
    (void)vnode;
    (void)offset;

    const char *str = reinterpret_cast<const char *>(buffer);
    auto &vga = vga::the();

    for (size_t i = 0; i < size; ++i)
        vga.put_char(str[i]);

    return static_cast<int>(size);
}

int ConsoleDevice::read(VNode *vnode, void *buffer, size_t size, size_t offset)
{
    (void)vnode;
    (void)offset;

    char *buf = reinterpret_cast<char *>(buffer);
    size_t bytes = 0;

    while (bytes < size && PS2Keyboard::the().has_key())
    {
        buf[bytes++] = PS2Keyboard::the().pop_key();
    }

    return static_cast<int>(bytes);
}

VNodeOps ConsoleDevice::ops = {
    .read = &ConsoleDevice::read,
    .write = &ConsoleDevice::write,
    .open = &ConsoleDevice::open,
    .close = &ConsoleDevice::close,
    .lookup = nullptr,
    .create = nullptr,
    .readdir = nullptr,
    .unlink = nullptr,
};
