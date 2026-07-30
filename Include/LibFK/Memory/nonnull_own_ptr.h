#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/own_ptr.h>

namespace fk {
namespace memory {

template <typename T>
class NonnullOwnPtr {
public:
    NonnullOwnPtr() = delete;

    explicit NonnullOwnPtr(T* ptr) : m_ptr(ptr) {
        if (m_ptr == nullptr)
            fk::algorithms::kwarn("NONNULL", "NonnullOwnPtr constructed with null");
    }

    NonnullOwnPtr(const NonnullOwnPtr&) = delete;
    NonnullOwnPtr& operator=(const NonnullOwnPtr&) = delete;

    NonnullOwnPtr(NonnullOwnPtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    NonnullOwnPtr& operator=(NonnullOwnPtr&& other) noexcept {
        if (this != &other) {
            clear();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    ~NonnullOwnPtr() { clear(); }

    T* get() const { return m_ptr; }
    T* ptr() const { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }

    T* leak_ptr() {
        T* p = m_ptr;
        m_ptr = nullptr;
        return p;
    }

    OwnPtr<T> release() { return OwnPtr<T>(leak_ptr()); }

private:
    void clear() {
        if (m_ptr) { delete m_ptr; m_ptr = nullptr; }
    }

    T* m_ptr;
};

template <typename T, typename... Args>
inline NonnullOwnPtr<T> make_nonnull(Args&&... args) {
    T* ptr = new T(static_cast<Args&&>(args)...);
    if (ptr == nullptr)
        fk::algorithms::kwarn("NONNULL", "make_nonnull: allocation failed");
    return NonnullOwnPtr<T>(ptr);
}

} // namespace memory

using memory::NonnullOwnPtr;
using memory::make_nonnull;

} // namespace fk
