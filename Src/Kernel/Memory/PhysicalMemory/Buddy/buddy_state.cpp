#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Algorithms/log.h>

void BuddyState::reset() {
    for (size_t i = 0; i < NUM_ORDERS; ++i)
        m_free_lists[i] = nullptr;
}

void BuddyState::push(size_t idx, uintptr_t phys) {
    FreeBlock* block = reinterpret_cast<FreeBlock*>(KERNEL_VIRT_BASE + phys);
    block->phys_addr = phys;
    block->next = m_free_lists[idx];
    m_free_lists[idx] = block;
}

uintptr_t BuddyState::pop(size_t idx) {
    FreeBlock* head = m_free_lists[idx];
    if (!head)
        return 0;

    m_free_lists[idx] = head->next;
    return head->phys_addr;
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
