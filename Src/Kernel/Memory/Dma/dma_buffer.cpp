#include <Kernel/Memory/Dma/dma_buffer.h>
#include <Kernel/Memory/PhysicalMemory/physical_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/virtual_memory_manager.h>
#include <Kernel/Memory/VirtualMemory/Pages/page_flags.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/memory.h>

namespace {

struct DmaVaddrRange {
  uintptr_t vaddr;
  size_t pages;
};

static constexpr size_t MAX_DMA_FREE_RANGES = 128;
static DmaVaddrRange s_free_ranges[MAX_DMA_FREE_RANGES];
static size_t s_free_count = 0;
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

static uintptr_t alloc_dma_vaddr(size_t pages) {
  for (size_t i = 0; i < s_free_count; i++) {
    if (s_free_ranges[i].pages >= pages) {
      uintptr_t vaddr = s_free_ranges[i].vaddr;
      if (s_free_ranges[i].pages > pages) {
        s_free_ranges[i].vaddr += pages * PAGE_SIZE;
        s_free_ranges[i].pages -= pages;
      } else {
        s_free_ranges[i] = s_free_ranges[--s_free_count];
      }
      return vaddr;
    }
  }

  uintptr_t vaddr = s_next_vaddr;
  s_next_vaddr += pages * PAGE_SIZE;
  if (s_next_vaddr > DMA_REGION_BASE + DMA_REGION_SIZE) return 0;
  return vaddr;
}

static void free_dma_vaddr(uintptr_t vaddr, size_t pages) {
  for (size_t i = 0; i < s_free_count; i++) {
    if (s_free_ranges[i].vaddr + s_free_ranges[i].pages * PAGE_SIZE == vaddr) {
      s_free_ranges[i].pages += pages;
      return;
    }
    if (vaddr + pages * PAGE_SIZE == s_free_ranges[i].vaddr) {
      s_free_ranges[i].vaddr = vaddr;
      s_free_ranges[i].pages += pages;
      return;
    }
  }

  if (s_free_count >= MAX_DMA_FREE_RANGES) {
    fk::algorithms::kwarn("DMA", "Free vaddr range list full, leaking vaddr range");
    return;
  }

  s_free_ranges[s_free_count].vaddr = vaddr;
  s_free_ranges[s_free_count].pages = pages;
  s_free_count++;
}

} // anonymous namespace

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

  uintptr_t vaddr = alloc_dma_vaddr(alloc_pages);
  if (vaddr == 0) {
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

  free_dma_vaddr(reinterpret_cast<uintptr_t>(buffer.vaddr), page_count);

  buffer.vaddr = nullptr;
  buffer.phys = 0;
  buffer.size = 0;
}
