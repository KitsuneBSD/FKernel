#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyAllocator.h>
#include <LibFK/Utilities/size_checking.h>
#include <LibFK/Algorithms/log.h>

size_t BuddyAllocator::order_to_index(size_t order) const
{
    fk::algorithms::kdebug("BUDDY", "Converting order %zu to index", order);
    return order - MIN_ORDER;
}

uintptr_t BuddyAllocator::block_address(void* ptr) const
{
    fk::algorithms::kdebug("BUDDY", "Calculating block address for pointer: %p", ptr);
    return reinterpret_cast<uintptr_t>(ptr);
}

uintptr_t BuddyAllocator::buddy_of(uintptr_t address, size_t order) const
{
    fk::algorithms::kdebug("BUDDY", "Calculating buddy of address: 0x%lx, order: %zu", address, order);
    return address ^ (1ull << order);
}

bool BuddyAllocator::in_range(uintptr_t address) const
{
    fk::algorithms::kdebug("BUDDY", "Checking if address 0x%lx is in range [0x%lx, 0x%lx)", 
        address, m_base_address, m_base_address + m_length);
    return address >= m_base_address && address < (m_base_address + m_length);
}

void BuddyAllocator::push_free_block(size_t order, uintptr_t address)
{
    fk::algorithms::kdebug("BUDDY", "Pushing free block at address: 0x%lx, order: %zu", address, order);
    auto* block = reinterpret_cast<FreeBlock*>(address);
    block->next = m_free_lists[order_to_index(order)];
    m_free_lists[order_to_index(order)] = block;
}

uintptr_t BuddyAllocator::pop_free_block(size_t order)
{
    fk::algorithms::kdebug("BUDDY", "Popping free block of order: %zu", order);
    auto*& head = m_free_lists[order_to_index(order)];
    if (!head) {
        fk::algorithms::kdebug("BUDDY", "No free block available for order: %zu", order);
        return 0; 
    }

    uintptr_t address = reinterpret_cast<uintptr_t>(head);
    head = head->next;
    fk::algorithms::kdebug("BUDDY", "Popped free block at address: 0x%lx", address);
    return address;
}

void BuddyAllocator::initialize() {
    uintptr_t aligned_base = fk::utilities::align_up(m_base_address, PAGE_SIZE);
    uintptr_t end = m_base_address + m_length;

    m_base_address = aligned_base;
    m_length = end - aligned_base;

    uintptr_t current_address = m_base_address;
    size_t remaining_length = m_length;

    while (remaining_length > order_to_size(MIN_ORDER)) {
        size_t order = MAX_ORDER;

        while (order > MIN_ORDER) {
            size_t block_size = order_to_size(order);
            if ((current_address % block_size) == 0  && block_size <= remaining_length) {
                break; 
            }
            order--;
        }

        push_free_block(order, current_address);
        current_address += order_to_size(order);
        remaining_length -= order_to_size(order);
    }

    fk::algorithms::klog("BUDDY", "BuddyAllocator initialized: base_address=0x%lx, length=%zu", m_base_address, m_length);
}

void* BuddyAllocator::alloc(size_t order){
    if (order > MAX_ORDER) {
        fk::algorithms::kexception("BUDDY", "Allocation order %zu exceeds MAX_ORDER %zu", order, MAX_ORDER);
        return nullptr;
    }

    if (order < MIN_ORDER) {
        fk::algorithms::kdebug("BUDDY", "Requested order %zu is less than MIN_ORDER %zu, adjusting", order, MIN_ORDER);
        order = MIN_ORDER;
    }

    size_t index = order_to_index(order);

    if (m_free_lists[index] != nullptr){
        uintptr_t address = pop_free_block(order);
        return reinterpret_cast<void*>(address);
    }

    size_t higher_order = order + 1;
    for (; higher_order <= MAX_ORDER; higher_order++) {
        size_t higher_index = order_to_index(higher_order);
        if (m_free_lists[higher_index] != nullptr) {
            break;
        }
    }

    if (higher_order > MAX_ORDER) {
        fk::algorithms::kexception("BUDDY", "No suitable block found for order %zu", order);
        return nullptr;
    }

    uintptr_t address = pop_free_block(higher_order);

    for (size_t current_order = higher_order - 1; current_order >= order; current_order--) {
        uintptr_t buddy_address = address + order_to_size(current_order);
        push_free_block(current_order, buddy_address);
    }

    fk::algorithms::kdebug("BUDDY", "Allocated block at address: 0x%lx, order: %zu", address, order);
    return reinterpret_cast<void*>(address);
}

void BuddyAllocator::free(void* ptr, size_t order){
    if (!ptr){
        fk::algorithms::kexception("BUDDY", "Attempted to free a null pointer");
        return;
    }

    if (order <= MIN_ORDER){
        order = MIN_ORDER;
    }

    if (order > MAX_ORDER) {
        fk::algorithms::kexception("BUDDY", "Free order %zu exceeds MAX_ORDER %zu", order, MAX_ORDER);
        return;
    }

    uintptr_t address = block_address(ptr);

    while (order < MAX_ORDER) {
        uintptr_t buddy_address = buddy_of(address, order);

        if (!in_range(buddy_address)) {
            fk::algorithms::kdebug("BUDDY", "Buddy address 0x%lx out of range, stopping coalescence", buddy_address);
            break;
        }

        size_t index = order_to_index(order);
        FreeBlock* current = m_free_lists[index];
        FreeBlock* prev = nullptr;
        bool buddy_found = false;

        while (current) {
            if (reinterpret_cast<uintptr_t>(current) == buddy_address) {
                fk::algorithms::kdebug("BUDDY", "Found buddy at address: 0x%lx for coalescence", buddy_address);
                buddy_found = true;
                break;
            }
            prev = current;
            current = current->next;
        }

        if (!buddy_found) {
            fk::algorithms::kdebug("BUDDY", "No buddy found at address: 0x%lx, stopping coalescence", buddy_address);
            break;
        }

        if (prev) {
            prev->next = current->next;
        } else {
            m_free_lists[index] = current->next;
        }

        address = fk::utilities::min(address, buddy_address);
        order++;
    }

    push_free_block(order, address);
    fk::algorithms::kdebug("BUDDY", "Freed block at address: 0x%lx, order: %zu", address, order);
}