#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/MemoryManager/TlsfHeap.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

void TLSFHeap::expand(size_t bytes) {
  size_t pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
  uintptr_t base = m_heap_base + m_heap_size;

  if (m_heap_base == 0) {
    void *phys = PhysicalMemoryManager::the().alloc_physical_page(1);
    if (!phys) {
      return;
    }

    m_heap_base = reinterpret_cast<uintptr_t>(phys);
    m_heap_size = PAGE_SIZE;
    auto *block = reinterpret_cast<BlockHeader *>(m_heap_base);
    block->m_size = PAGE_SIZE - sizeof(BlockHeader);
    block->m_free = true;
    insert_block(block);

        fk::algorithms::klog(

            "TLSF", "Heap initialized at 0x%lx, size=%zu", m_heap_base,

            m_heap_size);

    
    return;
  }

  for (size_t i = 0; i < pages; ++i) {
    void *phys = PhysicalMemoryManager::the().alloc_physical_page(1);
    if (!phys) {
      fk::algorithms::kwarn("TLSF", "Failed to allocate physical page for heap expansion");
      return;
    }

    VirtualMemoryManager::the().map_page(base + i * PAGE_SIZE, (uintptr_t)phys,
                                         PageFlags::Present |
                                             PageFlags::Writable);
  }

  auto *block = reinterpret_cast<BlockHeader *>(base);
  block->m_size = pages * PAGE_SIZE - sizeof(BlockHeader);
  block->m_free = true;
  insert_block(block);

  m_heap_size += pages * PAGE_SIZE;
  fk::algorithms::kdebug("TLSF", "Heap expanded by %zu pages, new size=%zu", pages,
         m_heap_size);
}

void TLSFHeap::insert_block(BlockHeader *block) {
  size_t fl, sl;
  mapping(block->m_size, fl, sl);
  block->m_next = m_free_lists[fl][sl];
  if (block->m_next)
    block->m_next->m_prev = block;
  block->m_prev = nullptr;
  m_free_lists[fl][sl] = block;
  m_first_layer_bitmap.set(fl, true);
  m_second_layer_bitmap[fl].set(sl, true);
}

void TLSFHeap::remove_block(BlockHeader *block) {
  size_t fl, sl;
  mapping(block->m_size, fl, sl);

  if (block->m_prev)
    block->m_prev->m_next = block->m_next;
  else
    m_free_lists[fl][sl] = block->m_next;
  if (block->m_next)
    block->m_next->m_prev = block->m_prev;

  if (!m_free_lists[fl][sl]) {
    m_second_layer_bitmap[fl].set(sl, false);
    if (m_second_layer_bitmap[fl].is_empty())
      m_first_layer_bitmap.set(fl, false);
  }

  block->m_next = nullptr;
  block->m_prev = nullptr;
}

void *TLSFHeap::alloc(size_t size, size_t align) {
  if (!size || !align || (align & (align - 1)) != 0)
    return nullptr;

  // Ajusta tamanho mínimo e espaço para header
  size_t adjusted_size = size + sizeof(BlockHeader);
  if (adjusted_size < MinBlockSize)
    adjusted_size = MinBlockSize;

  // Tenta localizar um bloco livre
  BlockHeader *block = locate_free_block(adjusted_size);

  // Se não encontrou, expande o heap e tenta novamente
  if (!block) {
    expand(adjusted_size);
    block = locate_free_block(adjusted_size);

    // Fallback: aloca diretamente da PMM
    if (!block) {
      size_t pages = (adjusted_size + PAGE_SIZE - 1) / PAGE_SIZE;
      void *phys = PhysicalMemoryManager::the().alloc_physical_page(pages);
      if (!phys)
        return nullptr;
      block = reinterpret_cast<BlockHeader *>(phys);
      block->m_size = adjusted_size;
      block->m_free = false;
      return reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(block) +
                                      sizeof(BlockHeader));
    }
  }

  remove_block(block);

  // Split se o bloco for grande demais
  size_t total_size = block->m_size + sizeof(BlockHeader);
  size_t remainder = total_size - adjusted_size;
  if (remainder >= MinBlockSize) {
    auto *new_block = reinterpret_cast<BlockHeader *>(
        reinterpret_cast<uintptr_t>(block) + adjusted_size);
    new_block->m_size = remainder - sizeof(BlockHeader);
    new_block->m_free = true;
    insert_block(new_block);
    block->m_size = adjusted_size - sizeof(BlockHeader);
  }

  block->m_free = false;
  m_used += block->m_size + sizeof(BlockHeader);

  uintptr_t user_ptr = reinterpret_cast<uintptr_t>(block) + sizeof(BlockHeader);
  uintptr_t aligned_ptr = (user_ptr + align - 1) & ~(align - 1);

  return reinterpret_cast<void *>(aligned_ptr);
}

void TLSFHeap::free(void *ptr) {
  if (!ptr)
    return;

  auto *block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<uintptr_t>(ptr) - sizeof(BlockHeader));

  if (block->m_free)
    return; // double free

  block->m_free = true;
  m_used -= block->m_size + sizeof(BlockHeader);

  auto *next_block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<uintptr_t>(block) + sizeof(BlockHeader) + block->m_size);
  if (reinterpret_cast<uintptr_t>(next_block) < m_heap_base + m_heap_size &&
      next_block->m_free) {
    remove_block(next_block);
    block->m_size += next_block->m_size + sizeof(BlockHeader);
  }

  BlockHeader *prev_block = nullptr;
  uintptr_t scan = m_heap_base;
  while (scan < reinterpret_cast<uintptr_t>(block)) {
    auto *current = reinterpret_cast<BlockHeader *>(scan);
    if (current->m_free)
      prev_block = current;
    scan += current->m_size + sizeof(BlockHeader);
  }

  if (prev_block && prev_block->m_free) {
    remove_block(prev_block);
    prev_block->m_size += block->m_size + sizeof(BlockHeader);
    block = prev_block;
  }

  insert_block(block);
}

BlockHeader *TLSFHeap::locate_free_block(size_t size) {
  size_t fl, sl;
  mapping(size, fl, sl);

  uint64_t sl_map = m_second_layer_bitmap[fl].get_mask_from(sl);
  if (sl_map == 0) {
    uint64_t fl_map = m_first_layer_bitmap.get_mask_from(fl + 1);
    if (fl_map == 0) {
      return nullptr;
    }
    fl = __builtin_ctzll(fl_map);
    sl_map = m_second_layer_bitmap[fl].get_mask();
  }

  sl = __builtin_ctzll(sl_map);
  BlockHeader *block = m_free_lists[fl][sl];
  while (block && block->m_size < size) {
    block = block->m_next;
  }

  return block;
}

void *TLSFHeap::realloc(void *ptr, size_t size) {
  if (!ptr)
    return alloc(size);
  if (!size) {
    free(ptr);
    return nullptr;
  }

  auto *block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<uintptr_t>(ptr) - sizeof(BlockHeader));
  size_t current_size = block->m_size;

  if (size <= current_size) {
    size_t remainder = current_size - size;
    if (remainder >= MinBlockSize) {
      auto *new_block = reinterpret_cast<BlockHeader *>(
          reinterpret_cast<uintptr_t>(block) + sizeof(BlockHeader) + size);
      new_block->m_size = remainder - sizeof(BlockHeader);
      new_block->m_free = true;
      insert_block(new_block);
      block->m_size = size;
    }
    return ptr;
  }

  auto *next_block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<uintptr_t>(block) + sizeof(BlockHeader) + block->m_size);
  if (reinterpret_cast<uintptr_t>(next_block) < m_heap_base + m_heap_size &&
      next_block->m_free &&
      (block->m_size + sizeof(BlockHeader) + next_block->m_size) >= size) {
    remove_block(next_block);
    block->m_size += sizeof(BlockHeader) + next_block->m_size;
    return ptr;
  }

  void *new_ptr = alloc(size);
  if (!new_ptr)
    return nullptr;
  memcpy(new_ptr, ptr, current_size);
  free(ptr);
  return new_ptr;
}
