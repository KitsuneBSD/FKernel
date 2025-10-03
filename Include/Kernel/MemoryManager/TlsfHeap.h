#pragma once

#include <LibC/stddef.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Container/bitmap.h>
#include <LibFK/Traits/types.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

struct BlockHeader {
  BlockHeader *m_next{nullptr};
  BlockHeader *m_prev{nullptr};
  size_t m_size{0};
  bool m_free{true};
};

static constexpr size_t FL_BITS = 64;
static constexpr size_t SL_BITS = 64;
static constexpr size_t Alignment = alignof(max_align_t);
static constexpr size_t BlockOverhead = sizeof(BlockHeader);

inline void mapping(size_t size, size_t &first_layer, size_t &second_layer) {
  first_layer = 63 - __builtin_clzll(size);
  second_layer = (size >> (first_layer - 5)) & (SL_BITS - 1);
}

/**
 * @class TLSFHeap
 * @brief A memory manager implementing the Two-Level Segregate Fit (TLSF) algorithm.
 * 
 * This class provides efficient memory allocation and deallocation with constant-time
 * complexity for most operations. It is designed for systems requiring high performance
 * and low fragmentation.
 */
class TLSFHeap {
private:
    /**
     * @brief Bitmap for the first-level blocks.
     * 
     * This bitmap tracks the availability of blocks in the first-level segregated list.
     */
    Bitmap<uint64_t, FL_BITS> m_first_layer_bitmap;

    /**
     * @brief Bitmap for the second-level blocks.
     * 
     * Each entry in this array corresponds to a first-level block and tracks the availability
     * of blocks in the second-level segregated list.
     */
    Bitmap<uint64_t, SL_BITS> m_second_layer_bitmap[FL_BITS];

    /**
     * @brief Array of free lists for memory blocks.
     * 
     * Each entry corresponds to a specific combination of first-level and second-level indices.
     */
    BlockHeader *m_free_lists[FL_BITS][SL_BITS]{};

    /**
     * @brief Base address of the heap.
     */
    uintptr_t m_heap_base{0};

    /**
     * @brief Total size of the heap in bytes.
     */
    size_t m_heap_size{0};

    /**
     * @brief Amount of memory currently in use.
     */
    size_t m_used{0};

    /**
     * @brief Locate a free block that can satisfy a memory allocation request.
     * 
     * @param size The size of the memory block to allocate.
     * @return A pointer to a suitable free block, or nullptr if none is available.
     */
    BlockHeader *locate_free_block(size_t size);

    /**
     * @brief Insert a memory block into the free list.
     * 
     * @param block The block to insert.
     */
    void insert_block(BlockHeader *block);

    /**
     * @brief Remove a memory block from the free list.
     * 
     * @param block The block to remove.
     */
    void remove_block(BlockHeader *block);

    /**
     * @brief Private constructor to initialize the TLSFHeap.
     * 
     * This constructor sets up the initial heap structure and allocates the first memory region.
     */
    TLSFHeap()
    : m_first_layer_bitmap{}
    , m_heap_base(0)
    , m_heap_size(0)
    , m_used(0)
{
    for (size_t i = 0; i < FL_BITS; i++) {
        for (size_t j = 0; j < SL_BITS; j++) {
            m_free_lists[i][j] = nullptr;
        }
    }

    auto initial_heap_size = 16 * MiB;
    void* region = PhysicalMemoryManager::the().alloc_physical_page(1);
    if (region) {
        m_heap_base = reinterpret_cast<uintptr_t>(region);
        m_heap_size = initial_heap_size;

        // Cria o primeiro bloco que cobre todo o heap inicial
        auto* block = reinterpret_cast<BlockHeader*>(m_heap_base);
        block->m_size = m_heap_size - sizeof(BlockHeader);
        block->m_free = true;
        block->m_next = nullptr;
        block->m_prev = nullptr;

        insert_block(block);
    }
}

    /**
     * @brief Deleted copy constructor.
     * 
     * Prevents copying of the TLSFHeap instance.
     */
    TLSFHeap(const TLSFHeap &) = delete;

    /**
     * @brief Deleted copy assignment operator.
     * 
     * Prevents assignment of the TLSFHeap instance.
     */
    TLSFHeap &operator=(const TLSFHeap &) = delete;

public:
    /**
     * @brief Get the singleton instance of the TLSFHeap.
     * 
     * @return A reference to the TLSFHeap instance.
     */
    static TLSFHeap &the(){
        static TLSFHeap instance;
        return instance;
    }

    /**
     * @brief Allocate a memory block.
     * 
     * @param size The size of the memory block to allocate.
     * @param align The alignment requirement for the memory block (default is max alignment).
     * @return A pointer to the allocated memory block, or nullptr if allocation fails.
     */
    void *alloc(size_t size, size_t align = alignof(max_align_t));

    /**
     * @brief Free a previously allocated memory block.
     * 
     * @param ptr A pointer to the memory block to free.
     */
    void free(void *ptr);

    /**
     * @brief Reallocate a memory block to a new size.
     * 
     * @param ptr A pointer to the memory block to reallocate.
     * @param new_size The new size of the memory block.
     * @return A pointer to the reallocated memory block, or nullptr if reallocation fails.
     */
    void *realloc(void *ptr, size_t new_size);

    /**
     * @brief Expand the heap by a specified number of bytes.
     * 
     * @param bytes The number of bytes to expand the heap by.
     */
    void expand(size_t bytes);

    /**
     * @brief Dump the current state of the heap for debugging purposes.
     */
    void dump() const;
};