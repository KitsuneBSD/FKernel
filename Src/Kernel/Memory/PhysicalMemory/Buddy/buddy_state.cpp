#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <LibFK/Algorithms/log.h>

void BuddyState::reset() {
    m_block_index = 0;

    for (size_t i = 0; i < NUM_ORDERS; ++i)
        m_free_lists[i] = nullptr;

    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug("BUDDY STATE", "BuddyState reset");
    */
}

FreeBlock* BuddyState::allocate_node(uintptr_t phys) {
    if (m_block_index >= 16384) {
        fk::algorithms::kwarn(
            "BUDDY STATE",
            "ERROR: BuddyState node pool exhausted"
        );
        return nullptr;
    }

    FreeBlock* b = &m_block_pool[m_block_index++];
    b->phys_addr = phys;
    b->next = nullptr;

    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "BUDDY STATE",
        "Node allocated: phys=%p idx=%zu",
        phys,
        m_block_index - 1
    );
    */
    return b;
}

void BuddyState::push(size_t idx, FreeBlock* block) {
    block->next = m_free_lists[idx];
    m_free_lists[idx] = block;
  
  /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "BUDDY STATE",
        "Push: order=%zu phys=%p",
        idx + MIN_ORDER,
        block->phys_addr
    );
  */
}

FreeBlock* BuddyState::pop(size_t idx) {
    FreeBlock* head = m_free_lists[idx];
    if (!head)
        return nullptr;

    m_free_lists[idx] = head->next;

    /*TODO: Apply this log when we work with LogLevel
    fk::algorithms::kdebug(
        "BUDDY STATE",
        "Pop: order=%zu phys=%p",
        idx + MIN_ORDER,
        head->phys_addr
    );
    */

    return head;
}

bool BuddyState::remove(size_t idx, uintptr_t phys) {
    FreeBlock* cur = m_free_lists[idx];
    FreeBlock* prev = nullptr;

    while (cur) {
        if (cur->phys_addr == phys) {
            if (prev)
                prev->next = cur->next;
            else
                m_free_lists[idx] = cur->next;
        /*TODO: Apply this log when we work with LogLevel     
            fk::algorithms::kdebug(
                "BUDDY STATE",
                "Remove: order=%zu phys=%p",
                idx + MIN_ORDER,
                phys
            );
        */
            return true;
        }

        prev = cur;
        cur = cur->next;
    }

    return false;
}
