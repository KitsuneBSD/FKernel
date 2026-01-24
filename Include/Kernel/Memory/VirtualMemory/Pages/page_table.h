#pragma once 

#include <LibFK/Types/types.h>

/**

 * @struct PageTable

 * @brief Represents an x86_64 page table (PML4, PDPT, PD, or PT).

 * 

 * Must be 4KB aligned.

 */

struct PageTable{

    uint64_t entries[512]; ///< 512 entries, each 8 bytes.

}__attribute__((aligned(4096)));
