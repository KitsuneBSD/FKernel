#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_state.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Algorithms/Logging/log.h>

void BuddyState::reset() {
    for (size_t i = 0; i < NUM_ORDERS; ++i)
        m_free_lists[i] = nullptr;
}

void BuddyState::push(size_t idx, uintptr_t phys) {
    FreeBlock* block = reinterpret_cast<FreeBlock*>(KERNEL_VIRT_BASE + phys);
    block->phys_addr = phys;
    block->list_idx = idx;
    block->prev = nullptr;
    block->next = m_free_lists[idx];
    if (m_free_lists[idx])
        m_free_lists[idx]->prev = block;
    m_free_lists[idx] = block;
}

uintptr_t BuddyState::pop(size_t idx) {
    FreeBlock* head = m_free_lists[idx];
    if (!head)
        return 0;

    m_free_lists[idx] = head->next;
    if (head->next)
        head->next->prev = nullptr;
    return head->phys_addr;
}

bool BuddyState::remove(size_t idx, uintptr_t phys) {
    // Guard: if the list is empty there is nothing to remove and the HHDM may
    // not be mapped yet (called from alloc_page() before extend_direct_map()).
    if (!m_free_lists[idx]) return false;

    // Block address is always KERNEL_VIRT_BASE + phys — no scan needed.
    FreeBlock* block = reinterpret_cast<FreeBlock*>(KERNEL_VIRT_BASE + phys);
    if (block->phys_addr != phys) return false;
    if (block->list_idx != idx) return false;

    if (block->prev)
        block->prev->next = block->next;
    else
        m_free_lists[idx] = block->next;

    if (block->next)
        block->next->prev = block->prev;

    return true;
}
