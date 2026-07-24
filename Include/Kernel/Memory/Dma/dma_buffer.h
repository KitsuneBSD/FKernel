#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Core/result.h>
#include <LibFK/Core/error.h>

/**
 * @struct DmaBuffer
 * @brief A physically-contiguous DMA buffer with a dedicated virtual mapping.
 *
 * Unlike identity-mapped DMA buffers, these are mapped into a dedicated
 * virtual address range (DMA_REGION_BASE) that is part of the kernel PML4
 * and inherited by all child processes. Hardware gets the physical address;
 * the CPU gets a proper kernel virtual pointer.
 *
 * Per Intel SDM §11.4, DMA buffers should use uncacheable or write-combining
 * memory type to ensure coherence between CPU and device writes.
 */
struct DmaBuffer {
  void* vaddr = nullptr;
  uintptr_t phys = 0;
  size_t size = 0;
};

/**
 * @brief Allocates a physically-contiguous DMA buffer.
 *
 * Allocates physical pages, maps them into the dedicated DMA virtual region,
 * and returns both the virtual pointer and physical address. The buffer is
 * zeroed and marked uncacheable for DMA coherence.
 *
 * @param size Number of bytes to allocate (rounded up to page boundary).
 * @return DmaBuffer with valid vaddr/phys on success, or error.
 */
fk::core::Result<DmaBuffer, fk::core::Error> dma_alloc_buffer(size_t size);

/**
 * @brief Frees a previously allocated DMA buffer.
 *
 * Unmaps the virtual pages and frees the underlying physical memory.
 * The DmaBuffer is invalidated after this call.
 *
 * @param buffer The DmaBuffer to free. Must have been returned by dma_alloc_buffer.
 */
void dma_free_buffer(DmaBuffer& buffer);
