#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Maximum number of interrupt vectors for x86_64
 */
constexpr uint8_t MAX_X86_64_INTERRUPTS_LENGTH = 256;

/**
 * @brief Standard page size (4 KiB)
 */
constexpr size_t PAGE_SIZE = 4 * KiB;

/**
 * @brief Large page size (2 MiB)
 */
constexpr uintptr_t PAGE_SIZE_2M = 2 * MiB;

/**
 * @brief Mask to align addresses to the 4 KiB page boundary
 */
constexpr size_t PAGE_MASK = (~(PAGE_SIZE - 1));

/**
 * @brief Maximum number of memory chunks per range
 */
constexpr size_t MAX_CHUNKS_PER_RANGE = UINT64_MAX;

/**
 * @brief Size of each Interrupt Stack Table (IST) stack
 */
constexpr size_t IST_STACK_SIZE = 16 * KiB;

/**
 * @brief Selector for the 64-bit Task State Segment (TSS)
 */
constexpr uint16_t TSS_SELECTOR = 0x28;
