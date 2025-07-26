#pragma once

#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/enforce.h>

namespace FK {

template<typename T>
class Vector {
private:
    T* data_ = nullptr;
    LibC::size_t size_ = 0;
    LibC::size_t capacity_ = 0;

    LibC::size_t pages_for_bytes(LibC::size_t bytes) const noexcept
    {
        return (bytes + TOTAL_MEMORY_PAGE_SIZE - 1) / TOTAL_MEMORY_PAGE_SIZE;
    }

    LibC::size_t bytes_for_capacity(LibC::size_t capacity) const noexcept
    {
        return capacity * sizeof(T);
    }

    void grow_to(LibC::size_t min_capacity)
    {
        if (min_capacity <= capacity_)
            return;

        LibC::size_t new_capacity = capacity_ ? capacity_ : 16;
        while (new_capacity < min_capacity)
            new_capacity *= 2;

        LibC::size_t new_bytes = bytes_for_capacity(new_capacity);
        LibC::size_t new_pages = pages_for_bytes(new_bytes);

        LibC::uintptr_t phys_addr = MemoryManagement::PhysicalMemoryManager::instance().alloc_contiguous_pages(new_pages);
        FK::enforcef(phys_addr != 0, "FK::Vector: falha ao alocar memória física contígua");

        LibC::uintptr_t virt_addr = MemoryManagement::VirtualMemoryManager::instance().allocate_virtual_range(new_pages);
        FK::enforcef(virt_addr != 0, "FK::Vector: falha ao alocar região virtual contígua");

        for (LibC::size_t page = 0; page < new_pages; ++page) {
            LibC::uintptr_t va = virt_addr + page * TOTAL_MEMORY_PAGE_SIZE;
            LibC::uintptr_t pa = phys_addr + page * TOTAL_MEMORY_PAGE_SIZE;

            bool mapped = MemoryManagement::VirtualMemoryManager::instance().map_page(
                va, pa,
                MemoryManagement::PAGE_PRESENT | MemoryManagement::PAGE_RW);
            FK::enforcef(mapped, "FK::Vector: falha ao mapear página %zu", page);
        }

        T* new_data = reinterpret_cast<T*>(virt_addr);

        if (data_) {
            for (LibC::size_t i = 0; i < size_; ++i)
                new_data[i] = data_[i];

            LibC::size_t old_pages = pages_for_bytes(bytes_for_capacity(capacity_));
            for (LibC::size_t page = 0; page < old_pages; ++page) {
                LibC::uintptr_t old_va = reinterpret_cast<LibC::uintptr_t>(data_) + page * TOTAL_MEMORY_PAGE_SIZE;
                MemoryManagement::VirtualMemoryManager::instance().unmap_page(old_va);
            }

            LibC::uintptr_t old_pa = MemoryManagement::VirtualMemoryManager::instance().get_physical_address(reinterpret_cast<LibC::uintptr_t>(data_));
            MemoryManagement::PhysicalMemoryManager::instance().free_contiguous_pages(old_pa, old_pages);
        }

        data_ = new_data;
        capacity_ = new_capacity;
    }

public:
    Vector() = default;

    ~Vector()
    {
        if (data_) {
            LibC::size_t pages = pages_for_bytes(bytes_for_capacity(capacity_));
            for (LibC::size_t page = 0; page < pages; ++page) {
                MemoryManagement::VirtualMemoryManager::instance().unmap_page(reinterpret_cast<LibC::uintptr_t>(data_) + page * TOTAL_MEMORY_PAGE_SIZE);
            }
            MemoryManagement::PhysicalMemoryManager::instance().free_contiguous_pages(reinterpret_cast<LibC::uintptr_t>(data_), pages);
        }
    }

    Vector(Vector const&) = delete;
    Vector& operator=(Vector const&) = delete;

    LibC::size_t size() const noexcept { return size_; }
    LibC::size_t capacity() const noexcept { return capacity_; }
    bool empty() const noexcept { return size_ == 0; }

    T& operator[](LibC::size_t index)
    {
        FK::enforcef(index < size_, "FK::Vector index out of bounds");
        return data_[index];
    }

    T const& operator[](LibC::size_t index) const
    {
        FK::enforcef(index < size_, "FK::Vector index out of bounds");
        return data_[index];
    }

    void push_back(T const& value)
    {
        grow_to(size_ + 1);
        data_[size_++] = value;
    }

    void pop_back()
    {
        FK::enforcef(size_ > 0, "FK::Vector pop_back called on empty vector");
        --size_;
    }

    void clear() noexcept
    {
        size_ = 0;
    }

    T* data() noexcept { return data_; }
    T const* data() const noexcept { return data_; }
};

}
