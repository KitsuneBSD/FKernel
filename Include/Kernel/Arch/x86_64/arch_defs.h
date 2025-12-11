#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Maximum number of interrupt vectors for x86_64
 */
constexpr uint8_t MAX_X86_64_INTERRUPTS_LENGTH = 256;

/**
 * @brief Maximum number of memory chunks per range
 */
constexpr size_t MAX_CHUNKS_PER_RANGE = UINT64_MAX;

/**
 * @brief Size of each Interrupt Stack Table (IST) stack
 */
constexpr size_t IST_STACK_SIZE = 16 * fk::types::KiB;

/**
 * @brief Selector for the 64-bit Task State Segment (TSS)
 */
constexpr uint16_t TSS_SELECTOR = 0x28;

/**
 * @brief TSS Index for the 64-bit Global Descriptor Table
 */
constexpr size_t TSS_INDEX = TSS_SELECTOR >> 3; // 0x28 >> 3 = 5

/**
 * @brief Size of Kernel Stack
 */
constexpr size_t KERNEL_STACK_SIZE = 4096 * 4;

/**
 * @brief Define a page size as a 4096 KiB;
 */

constexpr size_t PAGE_SIZE = 4 * fk::types::KiB;

/**
 * @brief Define a large page size as 2 MiB;
 */

constexpr size_t PAGE_SIZE_2M = 2 * fk::types::MiB;

/**
 * @brief Define a Page Mask
 */
constexpr uintptr_t PAGE_MASK = ~(PAGE_SIZE - 1);

/**
 * @brief Max number of Zones
 */

constexpr size_t MAX_ZONES = 16;