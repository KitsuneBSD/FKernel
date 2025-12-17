#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyAllocator.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Utilities/aligner.h>

BuddyAllocator::BuddyAllocator()
    : m_base_address(0), m_length(0) {
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::klog("BUDDY", "Ctor (empty)");
    */
    m_state.reset();
}

BuddyAllocator::BuddyAllocator(uintptr_t base_address, size_t length)
    : m_base_address(base_address), m_length(length) {
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::klog(
        "BUDDY",
        "Ctor: base=%p len=%zu",
        base_address,
        length
    );
    */
    m_state.reset();
    initialize();
}

void BuddyAllocator::add_range(uintptr_t base_address, size_t length) {
      fk::algorithms::klog(
        "BUDDY",
        "Add range: base=%p len=%zu",
        base_address,
        length
    );
    m_base_address = base_address;
    m_length = length;
    m_state.reset();
    initialize();
}

size_t BuddyAllocator::order_to_index(size_t order) const {
    return order - MIN_ORDER;
}

uintptr_t BuddyAllocator::buddy_of(uintptr_t address, size_t order) const {
    return address ^ (1ull << order);
}

bool BuddyAllocator::in_range(uintptr_t address) const {
    return address >= m_base_address &&
           address < (m_base_address + m_length);
}

FreeBlock* BuddyAllocator::new_block(uintptr_t phys) {
    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "BUDDY",
        "New node request: phys=%p",
        phys
    );
    */
    return m_state.allocate_node(phys);
}

void BuddyAllocator::push_free_block(size_t order, uintptr_t address) {
    /*TODO: Apply this log when we work with LogLevel
     fk::algorithms::kdebug(
        "BUDDY",
        "Push free block: order=%zu phys=%p",
        order,
        address
    );
    */

    size_t idx = order_to_index(order);
    FreeBlock* block = new_block(address);
    if (!block) {
        fk::algorithms::kwarn("BUDDY", "Push failed: node pool exhausted");
        return;
    }

    m_state.push(idx, block);
}

uintptr_t BuddyAllocator::pop_free_block(size_t order) {
    size_t idx = order_to_index(order);
    FreeBlock* block = m_state.pop(idx);

    if (!block) {
        fk::algorithms::kwarn(
            "BUDDY",
            "Pop failed: order=%zu",
            order
        );
        return 0;
    }

    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "BUDDY",
        "Pop free block: order=%zu phys=%p",
        order,
        block->phys_addr
    );
    */

    return block->phys_addr;
}

void BuddyAllocator::initialize() {
    fk::algorithms::klog(
        "BUDDY",
        "Initialize start: base=%p len=%zu",
        m_base_address,
        m_length
    );

    uintptr_t aligned = fk::utilities::align_up(
        m_base_address,
        BUDDY_PAGE_SIZE
    );

    uintptr_t end = m_base_address + m_length;

    m_base_address = aligned;
    m_length = end - aligned;

    uintptr_t current = m_base_address;
    size_t remaining = m_length;

    while (remaining >= order_to_size(MIN_ORDER)) {
        size_t order = MAX_ORDER;

        while (order > MIN_ORDER) {
            size_t size = order_to_size(order);
            if ((current & (size - 1)) == 0 && size <= remaining)
                break;
            order--;
        }

        push_free_block(order, current);

        size_t size = order_to_size(order);
        current += size;
        remaining -= size;
    }

    fk::algorithms::klog(
        "BUDDY",
        "Initialize done: base=%p len=%zu",
        m_base_address,
        m_length
    );
}

void* BuddyAllocator::alloc(size_t order) {
  /*TODO: Apply this log when we work with LogLevel
  fk::algorithms::kdebug(
        "BUDDY",
        "Alloc request: order=%zu",
        order
    );
  */

    if (order < MIN_ORDER)
        order = MIN_ORDER;
    if (order > MAX_ORDER)
        return nullptr;

    size_t cur = order;

    while (cur <= MAX_ORDER &&
           !m_state.m_free_lists[order_to_index(cur)])
        cur++;

    if (cur > MAX_ORDER) {
        fk::algorithms::kwarn(
            "BUDDY",
            "Alloc failed: no block available (order=%zu)",
            order
        );
        return nullptr;
    }

    uintptr_t addr = pop_free_block(cur);

    while (cur > order) {
        cur--;
        uintptr_t buddy = addr + order_to_size(cur);
        push_free_block(cur, buddy);
    }
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "BUDDY",
        "Alloc success: order=%zu phys=%p",
        order,
        addr
    );
*/
    return reinterpret_cast<void*>(addr);
}

void BuddyAllocator::free(void* ptr, size_t order) {
  /*TODO: Apply this log when we work with LogLevel  
  fk::algorithms::kdebug(
        "BUDDY",
        "Free request: ptr=%p order=%zu",
        ptr,
        order
    );
*/
    if (!ptr)
        return;

    if (order < MIN_ORDER)
        order = MIN_ORDER;
    if (order > MAX_ORDER)
        return;

    uintptr_t addr = reinterpret_cast<uintptr_t>(ptr);

    while (order < MAX_ORDER) {
        uintptr_t buddy = buddy_of(addr, order);

        if (!in_range(buddy)) {
            fk::algorithms::kwarn(
                "BUDDY",
                "Merge stop: buddy out of range phys=%p",
                buddy
            );
            break;
        }

        size_t idx = order_to_index(order);

        if (!m_state.remove(idx, buddy)) {
            fk::algorithms::kwarn(
                "BUDDY",
                "Merge stop: buddy not free phys=%p",
                buddy
            );
            break;
        }

        addr = addr < buddy ? addr : buddy;
        order++;
/*TODO: Apply this log when we work with LogLevel
        fk::algorithms::kdebug(
            "BUDDY",
            "Merge success: new order=%zu phys=%p",
            order,
            addr
        );
    */
    }

    push_free_block(order, addr);
/*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "BUDDY",
        "Free done: order=%zu phys=%p",
        order,
        addr
    );
  */
}
