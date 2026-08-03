#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Memory/Allocators/new.h>

namespace fk {
namespace memory {

template <typename T>
class OwnPtr {
public:
    OwnPtr() : m_ptr(nullptr) {}
    explicit OwnPtr(T* ptr) : m_ptr(ptr) {}
    
    OwnPtr(const OwnPtr&) = delete;
    OwnPtr& operator=(const OwnPtr&) = delete;
    
    OwnPtr(OwnPtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    template <typename U>
    OwnPtr(OwnPtr<U>&& other) noexcept : m_ptr(other.leak_ptr()) {}
    
    OwnPtr& operator=(OwnPtr&& other) noexcept {
        if (this != &other) {
            clear();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    OwnPtr& operator=(decltype(nullptr)) {
        clear();
        return *this;
    }

    OwnPtr& operator=(T* ptr) {
        clear();
        m_ptr = ptr;
        return *this;
    }
    
    ~OwnPtr() {
        clear();
    }
    
    T* get() const { return m_ptr; }
    T* ptr() const { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    explicit operator bool() const { return m_ptr != nullptr; }
    
    T* leak_ptr() {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

    void clear() {
        if (m_ptr) delete m_ptr;
        m_ptr = nullptr;
    }

private:
    T* m_ptr;
};

// Array specialization
template <typename T>
class OwnPtr<T[]> {
public:
    OwnPtr() : m_ptr(nullptr) {}
    explicit OwnPtr(T* ptr) : m_ptr(ptr) {}
    
    OwnPtr(const OwnPtr&) = delete;
    OwnPtr& operator=(const OwnPtr&) = delete;
    
    OwnPtr(OwnPtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }
    
    OwnPtr& operator=(OwnPtr&& other) noexcept {
        if (this != &other) {
            clear();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    OwnPtr& operator=(decltype(nullptr)) {
        clear();
        return *this;
    }

    OwnPtr& operator=(T* ptr) {
        clear();
        m_ptr = ptr;
        return *this;
    }
    
    ~OwnPtr() {
        clear();
    }
    
    T* get() const { return m_ptr; }
    T* ptr() const { return m_ptr; }
    T& operator[](size_t index) const { return m_ptr[index]; }
    explicit operator bool() const { return m_ptr != nullptr; }
    
    T* leak_ptr() {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }

    void clear() {
        if (m_ptr) delete[] m_ptr;
        m_ptr = nullptr;
    }

private:
    T* m_ptr;
};

template <typename T, typename... Args>
inline OwnPtr<T> make_owned(Args&&... args) {
    return OwnPtr<T>(new T(static_cast<Args&&>(args)...));
}

} // namespace memory

using memory::OwnPtr;
using memory::make_owned;

} // namespace fk
