#pragma once 

#include <LibFK/Types/types.h>
#include <LibFK/Container/string.h>
#include <LibFK/Utilities/aligner.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/FreeBlocks.h>
#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyOrder.h>
#include <Kernel/Arch/x86_64/arch_defs.h>

class BuddyAllocator {
private:
    FreeBlock m_block_pool[16384];
    size_t m_block_index = 0;
    uintptr_t m_base_address; 
    size_t m_length;
protected:
    void initialize();
    
    size_t order_to_index(size_t order) const; 
    uintptr_t block_address(void* ptr) const;
    size_t largest_order_fit(size_t size) const; 
    uintptr_t buddy_of(uintptr_t address, size_t order) const; 
    bool in_range(uintptr_t address) const; 
    void push_free_block(size_t order, uintptr_t address);
    uintptr_t pop_free_block(size_t order);

    FreeBlock* new_block(uintptr_t phys) {
        FreeBlock* b = &m_block_pool[m_block_index++];
        b->phys_addr = phys;
        b->next = nullptr;
        return b;
    }

public: 
    BuddyAllocator()
        : m_base_address(0)
        , m_length(0)
    {
    }

    BuddyAllocator(uintptr_t base_address, size_t length) 
        : m_base_address(base_address)
        , m_length(length)
    {
        initialize();
    }

    void add_range(uintptr_t base_address, size_t length){
        m_base_address = base_address;
        m_length = length;

        initialize();
    }

    void* alloc(size_t order);
    void free(void* ptr, size_t order);

    uintptr_t base_address() const { return m_base_address; }
    size_t length() const { return m_length; }
};