#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

namespace fk {
namespace memory {

struct WeakLink {
    void* ptr{nullptr};
    size_t ref_count{0};

    void retain() { ref_count++; }
    void release() {
        if (--ref_count == 0) delete this;
    }
};

template <typename T>
class WeakPtr;

template <typename T>
class Weakable {
public:
    Weakable() = default;
    ~Weakable() {
        if (m_link) {
            m_link->ptr = nullptr;
            m_link->release();
        }
    }

    WeakPtr<T> make_weak_ptr();

protected:
    WeakLink* ensure_link() {
        if (!m_link) {
            m_link = new WeakLink();
            m_link->ptr = static_cast<T*>(this);
            m_link->retain();
        }
        return m_link;
    }

private:
    WeakLink* m_link{nullptr};

    Weakable(const Weakable&) = delete;
    Weakable& operator=(const Weakable&) = delete;
};

template <typename T>
class WeakPtr {
public:
    WeakPtr() = default;

    explicit WeakPtr(WeakLink* link) : m_link(link) {
        if (m_link) m_link->retain();
    }

    WeakPtr(const WeakPtr& other) : m_link(other.m_link) {
        if (m_link) m_link->retain();
    }

    WeakPtr(WeakPtr&& other) noexcept : m_link(other.m_link) {
        other.m_link = nullptr;
    }

    WeakPtr& operator=(const WeakPtr& other) {
        if (this != &other) {
            if (m_link) m_link->release();
            m_link = other.m_link;
            if (m_link) m_link->retain();
        }
        return *this;
    }

    WeakPtr& operator=(WeakPtr&& other) noexcept {
        if (this != &other) {
            if (m_link) m_link->release();
            m_link = other.m_link;
            other.m_link = nullptr;
        }
        return *this;
    }

    ~WeakPtr() {
        if (m_link) m_link->release();
    }

    T* try_resolve() const {
        return m_link ? static_cast<T*>(m_link->ptr) : nullptr;
    }

    bool is_null() const { return try_resolve() == nullptr; }
    explicit operator bool() const { return !is_null(); }

private:
    WeakLink* m_link{nullptr};
};

template <typename T>
WeakPtr<T> Weakable<T>::make_weak_ptr() {
    return WeakPtr<T>(ensure_link());
}

} // namespace memory

using memory::WeakPtr;
using memory::Weakable;

} // namespace fk
