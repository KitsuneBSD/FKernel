#pragma once

#include <LibC/stdint.h>

namespace Io {

inline void outb(LibC::uint16_t port, LibC::uint8_t val)
{
    asm volatile("outb %0, %1" : : "a"(val), "Nd"(port));
}

inline LibC::uint8_t inb(LibC::uint16_t port)
{
    LibC::uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outw(LibC::uint16_t port, LibC::uint16_t val)
{
    asm volatile("outw %0, %1" : : "a"(val), "Nd"(port));
}

inline LibC::uint16_t inw(LibC::uint16_t port)
{
    LibC::uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outl(LibC::uint16_t port, LibC::uint32_t val)
{
    asm volatile("outl %0, %1" : : "a"(val), "Nd"(port));
}

inline LibC::uint32_t inl(LibC::uint16_t port)
{
    LibC::uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

inline void outq(LibC::uint16_t port, LibC::uint64_t val)
{
    asm volatile("outq %0, %1" : : "a"(val), "Nd"(port));
}

inline LibC::uint64_t inq(LibC::uint16_t port)
{
    LibC::uint64_t ret;
    asm volatile("inq %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

}
