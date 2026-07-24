#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace arch {
namespace x86_64 {

static inline void io_wait() {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

static inline void outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline uint16_t inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

static inline void insw(uint16_t port, void* buffer, uint32_t count) {
    asm volatile("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

static inline void outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint32_t inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

static inline void invlpg(uintptr_t virt) {
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}

} // namespace x86_64
} // namespace arch
} // namespace fk
