#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Utilities/aligner.h>

#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.h>

BuddyAllocator::BuddyAllocator()
    : m_base_address(0), m_length(0) {
    m_state.reset();
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
}

void BuddyAllocator::initialize_from_bitmap(const fk::containers::Bitmap<uint64_t>& bitmap, uintptr_t zone_base) {
    fk::algorithms::klog(
        "BUDDY",
        "Initialize from bitmap: base=%p len=%zu zone_base=%p",
        m_base_address,
        m_length,
        zone_base
    );

    m_state.reset();

    uintptr_t aligned = fk::utilities::align_up(m_base_address, BUDDY_PAGE_SIZE);
    uintptr_t end = m_base_address + m_length;
    m_base_address = aligned;
    m_length = end - aligned;

    uintptr_t current = m_base_address;
    size_t remaining = m_length;

    while (remaining >= order_to_size(MIN_ORDER)) {
        size_t frame = (current - zone_base) / BUDDY_PAGE_SIZE;

        if (bitmap.get(frame)) {
            current += BUDDY_PAGE_SIZE;
            remaining -= BUDDY_PAGE_SIZE;
            continue;
        }

        size_t order = MAX_ORDER;
        while (order >= MIN_ORDER) {
            size_t size = order_to_size(order);
            if ((current & (size - 1)) != 0 || size > remaining) {
                order--;
                continue;
            }
            size_t base_frame = (current - zone_base) / BUDDY_PAGE_SIZE;
            size_t page_count = size / BUDDY_PAGE_SIZE;
            bool all_free = true;
            for (size_t p = 1; p < page_count; p++) {
                if (bitmap.get(base_frame + p)) {
                    all_free = false;
                    break;
                }
            }
            if (all_free) break;
            order--;
        }

        if (order < MIN_ORDER) {
            current += BUDDY_PAGE_SIZE;
            remaining -= BUDDY_PAGE_SIZE;
            continue;
        }

        push_free_block(order, current);

        size_t size = order_to_size(order);
        current += size;
        remaining -= size;
    }

    fk::algorithms::klog(
        "BUDDY",
        "Initialize from bitmap done: base=%p len=%zu",
        m_base_address,
        m_length
    );
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

void BuddyAllocator::push_free_block(size_t order, uintptr_t address) {
    size_t idx = order_to_index(order);
    m_state.push(idx, address);
    fk::algorithms::ktrace("BUDDY", "push order=%zu addr=%p", order, (void*)address);
}

uintptr_t BuddyAllocator::pop_free_block(size_t order) {
    size_t idx = order_to_index(order);
    uintptr_t phys = m_state.pop(idx);

    if (!phys) {
        fk::algorithms::kwarn(
            "BUDDY",
            "Pop failed: order=%zu",
            order
        );
        return 0;
    }

    fk::algorithms::ktrace("BUDDY", "pop order=%zu -> %p", order, (void*)phys);
    return phys;
}


void* BuddyAllocator::alloc(size_t order) {
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
        fk::algorithms::ktrace("BUDDY", "alloc split: order=%zu buddy=%p", cur, (void*)buddy);
        push_free_block(cur, buddy);
    }

    fk::algorithms::ktrace("BUDDY", "alloc(order=%zu) -> %p", order, (void*)addr);
    return reinterpret_cast<void*>(addr);
}

void BuddyAllocator::free(void* ptr, size_t order) {
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
            fk::algorithms::kdebug(
                "BUDDY",
                "Merge stop: buddy out of range phys=%p",
                buddy
            );
            break;
        }

        size_t idx = order_to_index(order);

        if (!m_state.remove(idx, buddy)) {
            fk::algorithms::kdebug(
                "BUDDY",
                "Merge stop: buddy not free phys=%p",
                buddy
            );
            break;
        }

        fk::algorithms::ktrace("BUDDY", "free merge: order=%zu addr=%p", order + 1, (void*)(addr < buddy ? addr : buddy));
        addr = addr < buddy ? addr : buddy;
        order++;
    }

    fk::algorithms::ktrace("BUDDY", "free(%p) final: push order=%zu", ptr, order);
    push_free_block(order, addr);
}

void BuddyAllocator::invalidate_page(uintptr_t phys) {
    // The page being invalidated may sit inside a larger free block.  Find the
    // maximal free block that contains phys and split it down until the page
    // is isolated; the sibling halves are re-inserted as free blocks so the
    // buddy and the bitmap stay consistent (no double-allocation).
    for (size_t order = MAX_ORDER; ; --order) {
        size_t size = order_to_size(order);
        uintptr_t block_base = phys & ~(size - 1);

        if (m_state.remove(order_to_index(order), block_base)) {
            while (order > MIN_ORDER) {
                --order;
                uintptr_t buddy = block_base + order_to_size(order);
                if (phys < buddy) {
                    push_free_block(order, buddy);
                } else {
                    push_free_block(order, block_base);
                    block_base = buddy;
                }
            }
            return;
        }

        if (order == MIN_ORDER)
            return;
    }
}
