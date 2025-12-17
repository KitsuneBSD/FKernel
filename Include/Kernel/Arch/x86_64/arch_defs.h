#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Maximum number of interrupt vectors for x86_64
 */
static constexpr uint8_t MAX_X86_64_INTERRUPTS_LENGTH = 256;

/**
 * @brief Maximum number of memory chunks per range
 */
static constexpr size_t MAX_CHUNKS_PER_RANGE = UINT64_MAX;

/**
 * @brief Size of each Interrupt Stack Table (IST) stack
 */
static constexpr size_t IST_STACK_SIZE = 16 * fk::types::KiB;

/**
 * @brief Selector for the 64-bit Task State Segment (TSS)
 */
static constexpr uint16_t TSS_SELECTOR = 0x28;

/**
 * @brief TSS Index for the 64-bit Global Descriptor Table
 */
static constexpr size_t TSS_INDEX = TSS_SELECTOR >> 3; // 0x28 >> 3 = 5

/**
 * @brief Size of Kernel Stack
 */
static constexpr size_t KERNEL_STACK_SIZE = 4096 * 4;

/**
 * @brief Define a page size as a 4096 KiB;
 */

static constexpr size_t PAGE_SIZE = 4 * fk::types::KiB;

/**
 * @brief Define a large page size as 2 MiB;
 */

static constexpr size_t PAGE_SIZE_2M = 2 * fk::types::MiB;

/**
 * @brief Define a Page Mask
 */
static constexpr uintptr_t PAGE_MASK = ~(PAGE_SIZE - 1);

/**
 * @brief Max number of Zones
 */
static constexpr size_t MAX_ZONES = 16;

/**
 * @brief Maximum number of physical frames supported by the DMA zone.
*/
static constexpr size_t MAX_DMA_FRAMES = 4 * fk::types::MiB;

/**
 * @brief Maximum number of physical frames supported by the NORMAL zone.
*/
static constexpr size_t MAX_NORMAL_FRAMES = 1 * fk::types::GiB;

/**
 * @brief Maximum number of physical frames supported by the HIGH zone.
*/
static constexpr size_t MAX_HIGH_FRAMES = 4 * fk::types::GiB;

/**
 * @brief Base virtual address for the kernel (higher-half mapping).
 */
static constexpr uintptr_t KERNEL_VIRT_BASE = 0xFFFF800000000000;

/**
 * @brief Base physical address of the kernel in memory.
 */
static constexpr uintptr_t KERNEL_PHYS_BASE = 1 * fk::types::MiB;

static constexpr uintptr_t INITIAL_IDENTITY_MAPPING_SIZE = 4 * fk::types::GiB;

static constexpr uintptr_t PML4_RECURSIVE_SLOT = 0x1FF;

static constexpr size_t MAX_TABLES = 512;
