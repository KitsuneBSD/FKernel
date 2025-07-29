#pragma once

#include <Kernel/MemoryManagement/PhysicalMemoryManagement/PhysicalMemoryManager.h>
#include <Kernel/MemoryManagement/VirtualMemoryManagement/VirtualMemoryManagement.h>
#include <LibC/stddef.h>
#include <LibFK/enforce.h>
#include <LibFK/new.h>

namespace FK {

template<typename T>
class Vector {
private:
    T* data_ = nullptr;
    LibC::size_t size_ = 0;
    LibC::size_t capacity_ = 0;

    static constexpr LibC::size_t page_size = TOTAL_MEMORY_PAGE_SIZE;

    static LibC::size_t pages_for_bytes(LibC::size_t bytes) noexcept
    {
        return (bytes + page_size - 1) / page_size;
    }

    static LibC::size_t bytes_for_capacity(LibC::size_t capacity) noexcept
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

        LibC::uintptr_t phys = MemoryManagement::PhysicalMemoryManager::instance().alloc_contiguous_pages(new_pages);
        FK::enforcef(phys != 0, "Vector: failed to allocate physical memory");

        LibC::uintptr_t virt = MemoryManagement::VirtualMemoryManager::instance().allocate_virtual_range(new_pages);
        FK::enforcef(virt != 0, "Vector: failed to allocate virtual range");

        for (LibC::size_t page = 0; page < new_pages; ++page) {
            bool mapped = MemoryManagement::VirtualMemoryManager::instance().map_page(
                virt + page * page_size,
                phys + page * page_size,
                MemoryManagement::PAGE_PRESENT | MemoryManagement::PAGE_RW);
            FK::enforcef(mapped, "Vector: failed to map page %zu", page);
        }

        T* new_data = reinterpret_cast<T*>(virt);

        for (LibC::size_t i = 0; i < size_; ++i) {
            new (&new_data[i]) T(static_cast<T&&>(data_[i]));
            data_[i].~T();
        }

        if (data_)
            free_memory();

        data_ = new_data;
        capacity_ = new_capacity;
    }

    void free_memory()
    {
        if (!data_)
            return;

        LibC::uintptr_t va = reinterpret_cast<LibC::uintptr_t>(data_);
        LibC::uintptr_t pa = MemoryManagement::VirtualMemoryManager::instance().get_physical_address(va);
        LibC::size_t pages = pages_for_bytes(bytes_for_capacity(capacity_));

        for (LibC::size_t i = 0; i < pages; ++i)
            MemoryManagement::VirtualMemoryManager::instance().unmap_page(va + i * page_size);

        MemoryManagement::PhysicalMemoryManager::instance().free_contiguous_pages(pa, pages);
    }

public:
    Vector() = default;

    ~Vector()
    {
        for (LibC::size_t i = 0; i < size_; ++i)
            data_[i].~T();
        free_memory();
    }

    Vector(Vector const&) = delete;
    Vector& operator=(Vector const&) = delete;

    template<typename... Args>
    void emplace_back(Args&&... args)
    {
        grow_to(size_ + 1);
        new (&data_[size_]) T(static_cast<Args&&>(args)...);
        ++size_;
    }

    void remove_at(LibC::size_t index)
    {
        FK::enforcef(index < size_, "Vector: remove_at out-of-bounds (index %zu, size %zu)", index, size_);

        data_[index].~T();

        for (LibC::size_t i = index + 1; i < size_; ++i) {
            new (&data_[i - 1]) T(static_cast<T&&>(data_[i]));
            data_[i].~T();
        }

        --size_;
    }

    Vector(Vector&& other) noexcept
    {
        data_ = other.data_;
        size_ = other.size_;
        capacity_ = other.capacity_;

        other.data_ = nullptr;
        other.size_ = 0;
        other.capacity_ = 0;
    }

    Vector& operator=(Vector&& other) noexcept
    {
        if (this != &other) {
            for (LibC::size_t i = 0; i < size_; ++i)
                data_[i].~T();
            free_memory();

            data_ = other.data_;
            size_ = other.size_;
            capacity_ = other.capacity_;

            other.data_ = nullptr;
            other.size_ = 0;
            other.capacity_ = 0;
        }
        return *this;
    }

    void push_back(T const& value)
    {
        grow_to(size_ + 1);
        new (&data_[size_]) T(value);
        ++size_;
    }

    void push_back(T&& value)
    {
        grow_to(size_ + 1);
        new (&data_[size_]) T(static_cast<T&&>(value));
        ++size_;
    }

    void clear()
    {
        for (LibC::size_t i = 0; i < size_; ++i)
            data_[i].~T();
        size_ = 0;
    }

    void resize(LibC::size_t new_size)
    {
        if (new_size < size_) {
            for (LibC::size_t i = new_size; i < size_; ++i)
                data_[i].~T();
        } else {
            grow_to(new_size);
            for (LibC::size_t i = size_; i < new_size; ++i)
                new (&data_[i]) T(); // default construct
        }
        size_ = new_size;
    }

    void pop_back()
    {
        FK::enforcef(size_ > 0, "Vector: pop_back from empty vector");
        --size_;
        data_[size_].~T();
    }

    T& operator[](LibC::size_t index)
    {
        FK::enforcef(index < size_, "Vector: out of bounds access");
        return data_[index];
    }

    T const& operator[](LibC::size_t index) const
    {
        FK::enforcef(index < size_, "Vector: out of bounds access");
        return data_[index];
    }

    LibC::size_t size() const noexcept { return size_; }
    LibC::size_t capacity() const noexcept { return capacity_; }
    T* data() noexcept { return data_; }
    T const* data() const noexcept { return data_; }

    class Iterator {
        T* ptr_;

    public:
        explicit Iterator(T* ptr) noexcept
            : ptr_(ptr)
        {
        }

        T& operator*() const noexcept { return *ptr_; }
        T* operator->() const noexcept { return ptr_; }

        Iterator& operator++() noexcept
        {
            ++ptr_;
            return *this;
        }

        Iterator operator++(int) noexcept
        {
            Iterator tmp = *this;
            ++(*this);
            return tmp;
        }

        bool operator==(Iterator const& other) const noexcept { return ptr_ == other.ptr_; }
        bool operator!=(Iterator const& other) const noexcept { return ptr_ != other.ptr_; }
    };

    Iterator begin() noexcept { return Iterator(data_); }
    Iterator end() noexcept { return Iterator(data_ + size_); }
};

} // namespace FK
