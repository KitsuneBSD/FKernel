#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <LibFK/Algorithms/log.h>

void BuddyState::reset() {
    m_block_index = 0;

    for (size_t i = 0; i < NUM_ORDERS; ++i)
        m_free_lists[i] = nullptr;
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

    return b;
}

void BuddyState::push(size_t idx, FreeBlock* block) {
    block->next = m_free_lists[idx];
    m_free_lists[idx] = block;
}

FreeBlock* BuddyState::pop(size_t idx) {
    FreeBlock* head = m_free_lists[idx];
    if (!head)
        return nullptr;

    m_free_lists[idx] = head->next;

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

            return true;
        }

        prev = cur;
        cur = cur->next;
    }

    return false;
}
