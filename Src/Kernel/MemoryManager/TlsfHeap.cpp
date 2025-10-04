#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/MemoryManager/TlsfHeap.h>

#include <LibFK/Algorithms/log.h>
#include <LibFK/Traits/types.h>

#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <Kernel/MemoryManager/VirtualMemoryManager.h>

void TLSFHeap::expand(size_t bytes) {
  size_t pages = (bytes + PAGE_SIZE - 1) / PAGE_SIZE;
  uintptr_t base = m_heap_base + m_heap_size;

  for (size_t i = 0; i < pages; ++i) {
    void *phys_page = PhysicalMemoryManager::the().alloc_physical_page(1);

    if (!phys_page) {
      kwarn("TLSFHeap", "Failed to allocate physical page\n");
      return;
    }

    VirtualMemoryManager::the().map_page(
        base + i * PAGE_SIZE, (uintptr_t)phys_page,
        PageFlags::Present | PageFlags::Writable);
  }

  auto *block = reinterpret_cast<BlockHeader *>(base);
  block->m_size = pages * PAGE_SIZE - sizeof(BlockHeader);
  block->m_free = true;
  insert_block(block);

  m_heap_size += pages * PAGE_SIZE;
}

void TLSFHeap::insert_block(BlockHeader *block) {
  size_t fl, sl;
  mapping(block->m_size, fl, sl);

  block->m_next = m_free_lists[fl][sl];
  block->m_prev = nullptr;

  m_second_layer_bitmap[fl].set(sl, true);
  m_first_layer_bitmap.set(fl, true);

  block->m_next = m_free_lists[fl][sl];
  if (block->m_next)
    block->m_next->m_prev = block;
  block->m_prev = nullptr;
  m_free_lists[fl][sl] = block;

  klog("TLSFHeap", "Inserted block of size %lu at %p (FL: %lu, SL: %lu)",
       block->m_size, block, fl, sl);
}

void TLSFHeap::remove_block(BlockHeader *block) {
  size_t fl, sl;
  mapping(block->m_size, fl, sl);

  if (block->m_prev) {
    block->m_prev->m_next = block->m_next;
  } else {
    m_free_lists[fl][sl] = block->m_next;
  }

  if (block->m_next) {
    block->m_next->m_prev = block->m_prev;
  }

  if (!m_free_lists[fl][sl]) {
    m_second_layer_bitmap[fl].set(sl, false);
    if (m_second_layer_bitmap[fl].is_empty()) {
      m_first_layer_bitmap.set(fl, false);
    }
  }

  block->m_next = nullptr;
  block->m_prev = nullptr;

  klog("TLSFHeap", "Removed block of size %lu at %p (FL: %lu, SL: %lu)",
       block->m_size, block, fl, sl);
}

void TLSFHeap::dump() const {
  klog("TLSFHeap", "Heap Base: %p, Size: %lu, Used: %lu", (void *)m_heap_base,
       m_heap_size, m_used);
  for (size_t fl = 0; fl < FL_BITS; ++fl) {
    if (m_first_layer_bitmap.get(fl)) {
      for (size_t sl = 0; sl < SL_BITS; ++sl) {
        if (m_second_layer_bitmap[fl].get(sl)) {
          BlockHeader *block = m_free_lists[fl][sl];
          while (block) {
            klog("TLSFHeap", "Free Block - Address: %p, Size: %lu\n", block,
                 block->m_size);
            block = block->m_next;
          }
        }
      }
    }
  }
}

void* TLSFHeap::alloc(size_t size, size_t align) {
    if (size == 0 || align == 0 || (align & (align - 1)) != 0) {
        return nullptr;
    }

    // Ajusta para o tamanho mínimo, incluindo o header
    size_t adjusted_size = size + sizeof(BlockHeader);

    // Garante alinhamento para múltiplos de 2 do TLSF
    if (adjusted_size < sizeof(BlockHeader) + align) {
        adjusted_size = sizeof(BlockHeader) + align;
    }

    // Localiza o bloco livre adequado
    BlockHeader* block = locate_free_block(adjusted_size);
    if (!block) {
        expand(adjusted_size);
        block = locate_free_block(adjusted_size);
        if (!block) return nullptr;
    }

    remove_block(block);

    // Calcula o tamanho do bloco remanescente
    size_t total_block_size = block->m_size + sizeof(BlockHeader);
    size_t remainder = total_block_size - adjusted_size;

    // Só cria um novo bloco se o remanescente for grande o suficiente
    if (remainder >= sizeof(BlockHeader) + 2) { // mínimo para próximo tamanho TLSF
        auto* new_block = reinterpret_cast<BlockHeader*>(
            reinterpret_cast<uintptr_t>(block) + adjusted_size
        );
        new_block->m_size = remainder - sizeof(BlockHeader);
        new_block->m_free = true;
        insert_block(new_block);

        block->m_size = adjusted_size - sizeof(BlockHeader);
    }

    block->m_free = false;
    m_used += block->m_size + sizeof(BlockHeader);

    uintptr_t user_ptr = reinterpret_cast<uintptr_t>(block) + sizeof(BlockHeader);
    if (user_ptr % align != 0) {
        user_ptr += align - (user_ptr % align);
    }

    klog("TLSFHeap", "Allocated block of size %lu at %p", block->m_size, (void*)user_ptr);
    return reinterpret_cast<void*>(user_ptr);
}


void TLSFHeap::free(void *ptr) {
  if (!ptr) {
    return;
  }

  auto *block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<uintptr_t>(ptr) - sizeof(BlockHeader));

  if (block->m_free) {
    kwarn("TLSFHeap", "Double free detected at %p\n", ptr);
    return;
  }

  block->m_free = true;
  m_used -= block->m_size + sizeof(BlockHeader);

  auto *next_block = reinterpret_cast<BlockHeader *>(
      reinterpret_cast<uintptr_t>(block) + block->m_size + sizeof(BlockHeader));
  if (reinterpret_cast<uintptr_t>(next_block) < m_heap_base + m_heap_size &&
      next_block->m_free) {
    remove_block(next_block);
    block->m_size += next_block->m_size + sizeof(BlockHeader);
  }

  BlockHeader *prev_block = nullptr;
  uintptr_t scan = m_heap_base;
  while (scan < reinterpret_cast<uintptr_t>(block)) {
    auto *current = reinterpret_cast<BlockHeader *>(scan);
    if (current->m_free) {
      prev_block = current;
    }
    scan += current->m_size + sizeof(BlockHeader);
  }

  if (prev_block && prev_block->m_free) {
    remove_block(prev_block);
    prev_block->m_size += block->m_size + sizeof(BlockHeader);
    block = prev_block;
  }

  insert_block(block);

  klog("TLSFHeap", "Freed block at %p of size %lu", ptr, block->m_size);
}

BlockHeader* TLSFHeap::locate_free_block(size_t size) {
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