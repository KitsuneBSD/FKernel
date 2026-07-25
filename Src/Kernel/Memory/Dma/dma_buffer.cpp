#include <Kernel/Memory/Dma/dma_buffer.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Utilities/memory.h>

static uintptr_t s_next_vaddr = DMA_REGION_BASE;

static size_t to_buddy_order(size_t page_count) {
  size_t order = 0;
  size_t block = 1;
  while (block < page_count) {
    block <<= 1;
    order++;
  }
  return order;
}

fk::core::Result<DmaBuffer, fk::core::Error> dma_alloc_buffer(size_t size) {
  if (size == 0) {
    return fk::core::Error::InvalidParameter;
  }

  size_t page_count = (size + PAGE_SIZE - 1) / PAGE_SIZE;
  size_t order = to_buddy_order(page_count);
  size_t alloc_pages = static_cast<size_t>(1) << order;

  uintptr_t phys = PhysicalMemoryManager::the().alloc_contiguous(order);
  if (phys == 0) {
    return fk::core::Error::OutOfMemory;
  }

  uintptr_t vaddr = s_next_vaddr;
  s_next_vaddr += alloc_pages * PAGE_SIZE;

  if (s_next_vaddr > DMA_REGION_BASE + DMA_REGION_SIZE) {
    PhysicalMemoryManager::the().free_contiguous(phys, order);
    return fk::core::Error::OutOfMemory;
  }

  auto flags = static_cast<PageFlags>(
      PageFlags::Present | PageFlags::Writable | PageFlags::CacheDisabled);

  for (size_t i = 0; i < alloc_pages; i++) {
    VirtualMemoryManager::the().map_page(
        vaddr + i * PAGE_SIZE, phys + i * PAGE_SIZE, flags);
  }

  DmaBuffer buffer;
  buffer.vaddr = reinterpret_cast<void*>(vaddr);
  buffer.phys = phys;
  buffer.size = alloc_pages * PAGE_SIZE;

  fk::memory::set(buffer.vaddr, 0, buffer.size);

  return buffer;
}

void dma_free_buffer(DmaBuffer& buffer) {
  if (buffer.vaddr == nullptr || buffer.phys == 0) {
    return;
  }

  size_t page_count = buffer.size / PAGE_SIZE;

  for (size_t i = 0; i < page_count; i++) {
    VirtualMemoryManager::the().unmap_page(
        reinterpret_cast<uintptr_t>(buffer.vaddr) + i * PAGE_SIZE);
  }

  size_t order = to_buddy_order(page_count);
  PhysicalMemoryManager::the().free_contiguous(buffer.phys, order);

  buffer.vaddr = nullptr;
  buffer.phys = 0;
  buffer.size = 0;
}
