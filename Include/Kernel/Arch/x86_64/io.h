#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Small delay for I/O operations
 *
 * Used to ensure that previous I/O instructions have been processed
 * by the hardware before issuing the next one.
 */
static inline void io_wait() {
    asm volatile("outb %%al, $0x80" : : "a"(0));
}

/**
 * @brief Write a byte to an I/O port
 *
 * @param port Port number
 * @param value Byte value to write
 */
static inline void arch_outb(uint16_t port, uint8_t value) {
    asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Read a byte from an I/O port
 *
 * @param port Port number
 * @return Byte read from the port
 */
static inline uint8_t arch_inb(uint16_t port) {
    uint8_t ret;
    asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Read a word (2 bytes) from an I/O port
 *
 * @param port Port number
 * @return Word read from the port
 */
static inline uint16_t arch_inw(uint16_t port) {
    uint16_t ret;
    asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Write a word (2 bytes) to an I/O port
 *
 * @param port Port number
 * @param value Word value to write
 */
static inline void arch_outw(uint16_t port, uint16_t value) {
    asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Read multiple words from an I/O port
 *
 * @param port Port number
 * @param buffer Destination buffer
 * @param count Number of words to read
 */
static inline void arch_insw(uint16_t port, void *buffer, uint32_t count) {
    asm volatile("rep insw" : "+D"(buffer), "+c"(count) : "d"(port) : "memory");
}

/**
 * @brief Write a double word (4 bytes) to an I/O port
 *
 * @param port Port number
 * @param value Double word value to write
 */
static inline void arch_outl(uint16_t port, uint32_t value) {
    asm volatile("outl %0, %1" : : "a"(value), "Nd"(port));
}

/**
 * @brief Read a double word (4 bytes) from an I/O port
 *
 * @param port Port number
 * @return Double word read from the port
 */
static inline uint32_t arch_inl(uint16_t port) {
    uint32_t ret;
    asm volatile("inl %1, %0" : "=a"(ret) : "Nd"(port));
    return ret;
}

/**
 * @brief Invalidate a single Translation Lookaside Buffer (TLB) entry.
 *
 * This function issues the 'invlpg' instruction, which invalidates
 * the TLB entry for the specified virtual address. This is necessary
 * after modifying page table entries to ensure the CPU uses the
 * updated mapping.
 *
 * @param virt The virtual address for which the TLB entry should be invalidated.
 */
static inline void invlpg(uintptr_t virt) {
    asm volatile("invlpg (%0)" :: "r"(virt) : "memory");
}
