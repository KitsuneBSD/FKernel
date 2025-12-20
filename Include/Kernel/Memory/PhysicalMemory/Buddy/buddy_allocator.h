#pragma once

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif 
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_order.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/free_blocks.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Text/string.h>
#include <LibFK/Types/types.h>
#include <LibFK/Utilities/aligner.h>

class BuddyAllocator {
private:
  BuddyState m_state;

  uintptr_t m_base_address;
  size_t m_length;

protected:
  void initialize();

  size_t order_to_index(size_t order) const;
  uintptr_t buddy_of(uintptr_t address, size_t order) const;
  bool in_range(uintptr_t address) const;

  FreeBlock* new_block(uintptr_t phys);
  void push_free_block(size_t order, uintptr_t address);
  uintptr_t pop_free_block(size_t order);

public:
  BuddyAllocator();
  BuddyAllocator(uintptr_t base_address, size_t length);

  void add_range(uintptr_t base_address, size_t length);

  void* alloc(size_t order);
  void free(void* ptr, size_t order);
};
