#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyAllocator.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/FreeBlocks.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/aligner.h>

size_t BuddyAllocator::order_to_index(size_t order) const {
  return order - MIN_ORDER;
}

uintptr_t BuddyAllocator::block_address(void *ptr) const {
  return reinterpret_cast<uintptr_t>(ptr);
}

uintptr_t BuddyAllocator::buddy_of(uintptr_t address, size_t order) const {
  return address ^ (1ull << order);
}

bool BuddyAllocator::in_range(uintptr_t address) const {
  return address >= m_base_address && address < (m_base_address + m_length);
}

void BuddyAllocator::push_free_block(size_t order, uintptr_t address) {
  auto *block = new_block(address);
  size_t idx = order_to_index(order);

  block->next = &m_block_pool[idx];
  m_block_pool[idx] = *block;

  fk::algorithms::kdebug("BUDDY", "Push free block: order=%zu, phys=%p", order,
                         address);
}

uintptr_t BuddyAllocator::pop_free_block(size_t order) {
  size_t idx = order_to_index(order);
  FreeBlock *head = &m_block_pool[idx];

  if (!head)
    return 0;

  FreeBlock *blk = head;
  head = blk->next;

  fk::algorithms::kdebug("BUDDY", "Pop free block: order=%zu -> phys=%p", order,
                         blk->phys_addr);

  return blk->phys_addr;
}

void BuddyAllocator::initialize() {
  FreeBlock empty;
  for (size_t i = 0; i < NUM_ORDERS; i++)
    m_block_pool[i] = empty;

  m_block_index = 0;

  uintptr_t aligned = fk::utilities::align_up(m_base_address, PAGE_SIZE);
  uintptr_t end = m_base_address + m_length;

  m_base_address = aligned;
  m_length = end - aligned;

  uintptr_t current = m_base_address;
  size_t remaining = m_length;

  while (remaining >= order_to_size(MIN_ORDER)) {
    size_t order = MAX_ORDER;

    while (order > MIN_ORDER) {
      size_t size = order_to_size(order);

      if ((current % size) == 0 && size <= remaining)
        break;

      order--;
    }

    push_free_block(order, current);

    size_t block_size = order_to_size(order);
    current += block_size;
    remaining -= block_size;
  }

  fk::algorithms::klog("BUDDY", "Initialized: base=0x%lx len=%lu",
                       m_base_address, m_length);
}

void *BuddyAllocator::alloc(size_t order) {
  if (order < MIN_ORDER)
    order = MIN_ORDER;
  if (order > MAX_ORDER)
    return nullptr;

  size_t target = order;
  size_t cur = order;

  while (cur <= MAX_ORDER && m_block_pool[order_to_index(cur)].phys_addr != 0)
    cur++;

  if (cur > MAX_ORDER)
    return nullptr;

  uintptr_t addr = pop_free_block(cur);

  while (cur > target) {
    cur--;
    uintptr_t buddy = addr + order_to_size(cur);
    push_free_block(cur, buddy);
  }

  fk::algorithms::kdebug("BUDDY", "Alloc (order=%zu) -> phys=%p", order, addr);

  return reinterpret_cast<void *>(addr);
}

void BuddyAllocator::free(void *ptr, size_t order) {
  if (!ptr)
    return;

  if (order < MIN_ORDER)
    order = MIN_ORDER;
  if (order > MAX_ORDER)
    return;

  uintptr_t addr = block_address(ptr);

  while (order < MAX_ORDER) {
    uintptr_t buddy = buddy_of(addr, order);
    if (!in_range(buddy))
      break;

    size_t idx = order_to_index(order);
    FreeBlock *cur = &m_block_pool[idx];
    FreeBlock *prev = nullptr;
    bool found = false;

    while (cur) {
      if (cur->phys_addr == buddy) {
        found = true;
        break;
      }
      prev = cur;
      cur = cur->next;
    }

    if (!found)
      break;

    if (prev)
      prev->next = cur->next;
    else
      m_block_pool[idx] = *cur->next;

    addr = (addr < buddy ? addr : buddy);
    order++;
  }

  push_free_block(order, addr);

  fk::algorithms::kdebug("BUDDY", "Free (@ptr=%p, order=%zu)", addr, order);
}
