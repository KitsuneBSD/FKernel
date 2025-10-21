#pragma once
#include <Kernel/FileSystem/DevFS/devfs.h>
#include <Kernel/FileSystem/fd.h>
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <LibFK/Memory/retain_ptr.h>

struct SerialTTY
{
    static int open(VNode *vnode, FileDescriptor *fd, int flags)
    {
        (void)vnode;
        (void)fd;
        (void)flags;
        return 0; // Não precisa de nada especial
    }

    static int close(VNode *vnode, FileDescriptor *fd)
    {
        (void)fd;
        (void)vnode;
        return 0;
    }

    static int write(VNode *vnode, FileDescriptor *fd, const void *buf, size_t size, [[maybe_unused]] size_t offset)
    {
        (void)fd;
        (void)vnode;
        const char *str = reinterpret_cast<const char *>(buf);
        for (size_t i = 0; i < size; ++i)
            serial::write_char(str[i]);
        return static_cast<int>(size);
    }

    static int read(VNode *vnode, FileDescriptor *fd, void *buf, size_t size, size_t offset)
    {
        (void)vnode;
        (void)fd;
        (void)buf;
        (void)size;
        (void)offset;
        return 0; // Não implementado ainda, poderia usar buffer circular da UART
    }

    static VNodeOps ops;
};

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