#pragma once

#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/Pointers/ref_ptr.h>

namespace fk {
namespace memory {

template <typename T>
class NonnullRefPtr {
public:
    NonnullRefPtr() = delete;

    NonnullRefPtr(T* ptr) : m_ptr(ptr) {
        if (m_ptr == nullptr)
            fk::algorithms::kwarn("NONNULL", "NonnullRefPtr constructed with null");
    }

    NonnullRefPtr(const NonnullRefPtr& other) : m_ptr(other.m_ptr) {
        m_ptr->ref();
    }

    NonnullRefPtr(NonnullRefPtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }

    NonnullRefPtr& operator=(const NonnullRefPtr& other) {
        if (this != &other) {
            other.m_ptr->ref();
            m_ptr->unref();
            m_ptr = other.m_ptr;
        }
        return *this;
    }

    NonnullRefPtr& operator=(NonnullRefPtr&& other) noexcept {
        if (this != &other) {
            m_ptr->unref();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    ~NonnullRefPtr() {
        if (m_ptr) m_ptr->unref();
    }

    T* get() const { return m_ptr; }
    T* ptr() const { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }

    RefPtr<T> as_nullable() const { return RefPtr<T>(m_ptr); }

private:
    T* m_ptr;
};

} // namespace memory

using memory::NonnullRefPtr;

} // namespace fk
