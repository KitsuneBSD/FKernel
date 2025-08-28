#pragma once

#include <Kernel/MemoryManagement/FreeListAllocator/falloc.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stddef.h>

namespace FK {

template<typename K, typename V>
class HashMap {
private:
    struct Entry {
        K key;
        V value;
        bool occupied;
    };

    Entry* table_ = nullptr;
    LibC::size_t capacity_ = 0;
    LibC::size_t size_ = 0;

    static constexpr LibC::size_t page_size = TOTAL_MEMORY_PAGE_SIZE;

    static LibC::size_t pages_for_bytes(LibC::size_t bytes)
    {
        return (bytes + page_size - 1) / page_size;
    }

    static LibC::size_t next_power_of_two(LibC::size_t v)
    {
        v--;
        v |= v >> 1;
        v |= v >> 2;
        v |= v >> 4;
        v |= v >> 8;
        v |= v >> 16;
        v++;
        return v;
    }

    static LibC::uintptr_t allocate_memory(LibC::size_t bytes)
    {
        LibC::size_t pages = pages_for_bytes(bytes);
        auto& vmm = MemoryManagement::VirtualMemoryManager::instance();
        auto& pmm = MemoryManagement::PhysicalMemoryManager::instance();

        LibC::uintptr_t phys = pmm.alloc_contiguous_pages(pages);
        FK::enforcef(phys != 0, "HashMap: failed to allocate physical memory");

        LibC::uintptr_t virt = vmm.allocate_virtual_range(pages);
        FK::enforcef(virt != 0, "HashMap: failed to allocate virtual memory");

        for (LibC::size_t i = 0; i < pages; ++i) {
            bool mapped = vmm.map_page(
                virt + i * page_size,
                phys + i * page_size,
                MemoryManagement::PAGE_PRESENT | MemoryManagement::PAGE_RW);
            FK::enforcef(mapped, "HashMap: failed to map page");
        }

        return virt;
    }

    void grow()
    {
        LibC::size_t new_capacity = capacity_ ? capacity_ * 2 : 16;
        rehash(new_capacity);
    }

    void rehash(LibC::size_t new_capacity)
    {
        LibC::uintptr_t new_mem = allocate_memory(sizeof(Entry) * new_capacity);
        Entry* new_table = reinterpret_cast<Entry*>(new_mem);

        for (LibC::size_t i = 0; i < new_capacity; ++i)
            new_table[i].occupied = false;

        for (LibC::size_t i = 0; i < capacity_; ++i) {
            if (table_[i].occupied) {
                insert_internal(new_table, new_capacity, table_[i].key, static_cast<V&&>(table_[i].value));
                table_[i].value.~V();
                table_[i].key.~K();
            }
        }

        if (table_)
            free_memory();

        table_ = new_table;
        capacity_ = new_capacity;
    }

    void insert_internal(Entry* table, LibC::size_t cap, K const& key, V&& value)
    {
        LibC::size_t index = hash(key) % cap;
        while (table[index].occupied) {
            if (table[index].key == key) {
                table[index].value = static_cast<V&&>(value);
                return;
            }
            index = (index + 1) % cap;
        }
        new (&table[index].key) K(key);
        new (&table[index].value) V(static_cast<V&&>(value));
        table[index].occupied = true;
    }

    void free_memory()
    {
        LibC::uintptr_t va = reinterpret_cast<LibC::uintptr_t>(table_);
        LibC::uintptr_t pa = MemoryManagement::VirtualMemoryManager::instance().get_physical_address(va);
        LibC::size_t pages = pages_for_bytes(sizeof(Entry) * capacity_);

        for (LibC::size_t i = 0; i < pages; ++i)
            MemoryManagement::VirtualMemoryManager::instance().unmap_page(va + i * page_size);

        MemoryManagement::PhysicalMemoryManager::instance().free_contiguous_pages(pa, pages);
    }

    LibC::size_t hash(K const& key) const
    {
        // Simples DJB2-like
        // TODO: Separe calc of Hash in a our own LibFK to Hash using CRC32
        LibC::size_t h = 5381;
        LibC::uint8_t const* ptr = reinterpret_cast<LibC::uint8_t const*>(&key);
        for (LibC::size_t i = 0; i < sizeof(K); ++i)
            h = ((h << 5) + h) + ptr[i];
        return h;
    }

public:
    HashMap() = default;

    ~HashMap()
    {
        for (LibC::size_t i = 0; i < capacity_; ++i) {
            if (table_[i].occupied) {
                table_[i].key.~K();
                table_[i].value.~V();
            }
        }
        if (table_)
            free_memory();
    }

    void insert(K const& key, V const& value)
    {
        if (size_ * 2 >= capacity_)
            grow();
        insert_internal(table_, capacity_, key, static_cast<V&&>(V(value)));
        ++size_;
    }

    bool get(K const& key, V& out_value) const
    {
        LibC::size_t index = hash(key) % capacity_;
        for (LibC::size_t i = 0; i < capacity_; ++i) {
            LibC::size_t probe = (index + i) % capacity_;
            if (!table_[probe].occupied)
                return false;
            if (table_[probe].key == key) {
                out_value = table_[probe].value;
                return true;
            }
        }
        return false;
    }
};

}
