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
static constexpr size_t KERNEL_STACK_SIZE = 16 * fk::types::KiB;

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

/**
 * @brief Initial identity mapping size
 */
static constexpr uintptr_t INITIAL_IDENTITY_MAPPING_SIZE = 4 * fk::types::GiB;

/**
 * @brief Recursive slot in the PML4
 */
static constexpr uintptr_t PML4_RECURSIVE_SLOT = 0x1FF;

/**
 * @brief Maximum number of tables in the PML4
 */
static constexpr size_t MAX_TABLES = 512;

/**
 * @brief Page table entries per table
 */
static constexpr size_t PT_ENTRIES = 512;

/**
 * @brief Page table level count (PML4 → PDPT → PD → PT)
 */
static constexpr size_t PAGE_TABLE_LEVELS = 4;

/**
 * @brief Index mask for page table entries (9 bits)
 */
static constexpr size_t TABLE_INDEX_MASK = 0x1FF;

/**
 * @brief Page table index shift amounts
 */
static constexpr size_t PML4_INDEX_SHIFT = 39;
static constexpr size_t PDPT_INDEX_SHIFT = 30;
static constexpr size_t PD_INDEX_SHIFT   = 21;
static constexpr size_t PT_INDEX_SHIFT   = 12;

/**
 * @brief Physical address mask in page table entries (bits 12–51)
 */
static constexpr uintptr_t PHYSICAL_ADDRESS_MASK = 0x000FFFFFFFFFF000ULL;

/**
 * @brief Physical address mask for 2 MiB pages (bits 21–51)
 */
static constexpr uintptr_t PAGE_ADDRESS_MASK_2M = 0x000FFFFFFFE00000ULL;

/**
 * @brief Physical address mask for 1 GiB pages (bits 30–51)
 */
static constexpr uintptr_t PAGE_ADDRESS_MASK_1G = 0x000FFFFFC0000000ULL;

/**
 * @brief Page flags mask (lower 12 bits of a PTE)
 */
static constexpr uintptr_t PAGE_FLAGS_MASK = 0xFFFULL;

/**
 * @brief Maximum number of physical zones
 */
static constexpr size_t MAX_PHYSICAL_ZONES = 32;

/**
 * @brief Maximum number of CPUs supported
 */
static constexpr size_t MAX_CPUS = 32;

/**
 * @brief IDT entry count (x86_64 supports 256 vectors)
 */
static constexpr size_t IDT_ENTRIES = 256;

/**
 * @brief GDT code segment selector (kernel, Ring 0)
 */
static constexpr uint32_t GDT_KERNEL_CODE_SELECTOR = 0x08;

/**
 * @brief GDT data segment selector (kernel, Ring 0)
 */
static constexpr uint32_t GDT_KERNEL_DATA_SELECTOR = 0x10;

/**
 * @brief FXSAVE/FXRSTOR state size in bytes
 */
static constexpr size_t FX_STATE_SIZE = 512;

/**
 * @brief Default base address for user-space mmap regions
 */
static constexpr uintptr_t MMAP_DEFAULT_BASE = 0x40000000;

/**
 * @brief Virtual address range for DMA buffer mappings.
 *
 * DMA buffers reside in their own PML4 entry (264) isolated from the
 * higher-half direct map at PML4[256] (KERNEL_VIRT_BASE).  This prevents
 * page-table conflicts when map_page() creates 4 KiB PTEs inside 2 MiB
 * HugePage PD entries set up by extend_direct_map().
 *
 * Range: 0xFFFF840000000000 - 0xFFFF840040000000 (1 GiB)
 */
static constexpr uintptr_t DMA_REGION_BASE = 0xFFFF840000000000;
static constexpr size_t DMA_REGION_SIZE = 1 * fk::types::GiB;