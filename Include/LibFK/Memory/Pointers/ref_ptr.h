#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/Traits/type_traits.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

namespace fk {
namespace memory {

template <typename T>
class RefPtr {
public:
    enum AdoptTag { Adopt };

    RefPtr() : m_ptr(nullptr) {}
    RefPtr(T* ptr) : m_ptr(ptr) {
        if (m_ptr) m_ptr->ref();
    }
    
    RefPtr(AdoptTag, T* ptr) : m_ptr(ptr) {
    }
    
    RefPtr(const RefPtr& other) : m_ptr(other.m_ptr) {
        if (m_ptr) m_ptr->ref();
    }
    
    template <typename U, typename = typename fk::traits::enable_if_t<fk::traits::is_base_of_v<T, U>>>
    RefPtr(const RefPtr<U>& other) : m_ptr(other.get()) {
        if (m_ptr) m_ptr->ref();
    }
    
    RefPtr(RefPtr&& other) noexcept : m_ptr(other.m_ptr) {
        other.m_ptr = nullptr;
    }
    
    template <typename U, typename = typename fk::traits::enable_if_t<fk::traits::is_base_of_v<T, U>>>
    RefPtr(RefPtr<U>&& other) noexcept : m_ptr(other.leak_ptr()) {
    }
    
    ~RefPtr() {
        if (m_ptr) m_ptr->unref();
    }
    
    RefPtr& operator=(const RefPtr& other) {
        if (this != &other) {
            if (other.m_ptr) other.m_ptr->ref();
            if (m_ptr) m_ptr->unref();
            m_ptr = other.m_ptr;
        }
        return *this;
    }
    
    template <typename U, typename = typename fk::traits::enable_if_t<fk::traits::is_base_of_v<T, U>>>
    RefPtr& operator=(const RefPtr<U>& other) {
        if (static_cast<void*>(this) != static_cast<const void*>(&other)) {
            if (other.get()) other.get()->ref();
            if (m_ptr) m_ptr->unref();
            m_ptr = other.get();
        }
        return *this;
    }
    
    RefPtr& operator=(RefPtr&& other) noexcept {
        if (this != &other) {
            if (m_ptr) m_ptr->unref();
            m_ptr = other.m_ptr;
            other.m_ptr = nullptr;
        }
        return *this;
    }

    template <typename U, typename = typename fk::traits::enable_if_t<fk::traits::is_base_of_v<T, U>>>
    RefPtr& operator=(RefPtr<U>&& other) noexcept {
        if (m_ptr) m_ptr->unref();
        m_ptr = other.leak_ptr();
        return *this;
    }

    RefPtr& operator=(decltype(nullptr)) {
        clear();
        return *this;
    }
    
    T* leak_ptr() {
        T* ptr = m_ptr;
        m_ptr = nullptr;
        return ptr;
    }
    
    T* get() const { return m_ptr; }
    T* ptr() const { return m_ptr; }
    T* operator->() const { return m_ptr; }
    T& operator*() const { return *m_ptr; }
    explicit operator bool() const { return m_ptr != nullptr; }
    
    bool operator==(const RefPtr& other) const { return m_ptr == other.m_ptr; }
    bool operator!=(const RefPtr& other) const { return m_ptr != other.m_ptr; }

    void clear() {
        if (m_ptr) m_ptr->unref();
        m_ptr = nullptr;
    }

private:
    T* m_ptr;
};

template <typename T>
inline RefPtr<T> adopt_ref(T* ptr) {
    if (!ptr) return {};
    return RefPtr<T>(RefPtr<T>::Adopt, ptr);
}

template <typename T, typename... Args>
inline fk::core::Result<RefPtr<T>, fk::core::Error> make_ref(Args&&... args) {
    auto res = fk::memory::allocate(sizeof(T));
    if (res.is_error()) return res.error();
    T* obj = reinterpret_cast<T*>(res.value());
    new (obj) T(static_cast<Args&&>(args)...);
    return adopt_ref(obj);
}

} // namespace memory

// Export to fk namespace for convenience if needed, or just use as is.
using memory::adopt_ref;
using memory::make_ref;
using memory::RefPtr;

} // namespace fk
