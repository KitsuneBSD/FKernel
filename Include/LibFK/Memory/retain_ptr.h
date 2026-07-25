#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/error.h>
#include <LibFK/Core/result.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Traits/type_traits.h> // Added missing include
#include <LibFK/Types/types.h>

namespace fk {
namespace memory {

/**
 * @brief Freestanding smart pointer with internal reference counting.
 *
 * This pointer manages the lifetime of an object using an internal
 * reference counter. It automatically deletes the object when
 * the last reference is released.
 *
 * Fully freestanding: does not use standard `new`/`delete`.
 *
 * @tparam T Type of object being managed
 */
template <typename T> class RetainPtr {
public:
  /// @brief Tag to adopt a pointer without creating a new reference count
  enum class Adopt { Yes };

  /** @brief Default constructor. Constructs an empty RetainPtr. */
  RetainPtr() : m_ptr(nullptr), m_refcount(nullptr) {}

  /**
   * @brief Constructs RetainPtr managing a given pointer.
   * @param ptr Pointer to manage
   */
  explicit RetainPtr(T *ptr) : m_ptr(ptr) {
    if (m_ptr) {
      m_refcount = fk::memory::allocate<uint32_t>();
      if (m_refcount) {
        *m_refcount = 1;
      } else {
        // Clean up the pointer since we can't manage it
        m_ptr->~T();
        fk::memory::deallocate(m_ptr);
        m_ptr = nullptr;
        m_refcount = nullptr;
      }
    } else {
      m_refcount = nullptr;
    }
  }

  /**
   * @brief Adopt a pointer that was already heap-allocated.
   *
   * Allocates a fresh refcount at 1 without calling retain on the object.
   * Used by make_retain() to wrap a newly allocated object as the first owner.
   * @param Adopt Tag (Adopt::Yes)
   * @param ptr Pointer to adopt (caller must not delete it)
   */
  RetainPtr(Adopt, T *ptr) : m_ptr(ptr), m_refcount(nullptr) {
    if (m_ptr) {
      m_refcount = fk::memory::allocate<uint32_t>();
      if (m_refcount) {
        *m_refcount = 1;
      } else {
        m_ptr = nullptr;
      }
    }
  }

  /**
   * @brief Copy constructor for derived types.
   * Allows RetainPtr<Derived> -> RetainPtr<Base> conversion.
   */
  template <typename U,
            typename = fk::traits::enable_if_t<fk::traits::is_base_of_v<T, U>>>
  RetainPtr(const RetainPtr<U> &other) noexcept
      : m_ptr(other.get()), m_refcount(other.getRefCount()) {
    if (m_refcount)
      __sync_fetch_and_add(m_refcount, 1u);
  }

  /** @brief Move constructor. Transfers ownership from another RetainPtr. */
  RetainPtr(RetainPtr &&other) noexcept
      : m_ptr(other.m_ptr), m_refcount(other.m_refcount) {
    other.m_ptr = nullptr;
    other.m_refcount = nullptr;
  }

  /**
   * @brief Provide access to the refcount for derived->base conversions.
   *        Only intended to be used by RetainPtr.
   */
  uint32_t *getRefCount() const { return m_refcount; }

  /**
   * @brief Copy constructor. Increments the reference count.
   * @param other Another RetainPtr to copy
   */
  RetainPtr(const RetainPtr &other)
      : m_ptr(other.m_ptr), m_refcount(other.m_refcount) {
    if (m_refcount)
      __sync_fetch_and_add(m_refcount, 1u);
  }

  /** @brief Destructor. Releases the managed object if this is the last
   * reference. */
  ~RetainPtr() { clear(); }

  // --- Assignment operators ---

  /**
   * @brief Copy assignment operator. Increments the reference count.
   * @param other RetainPtr to copy from
   * @return Reference to this
   */
  RetainPtr &operator=(const RetainPtr &other) {
    if (this != &other) {
      if (other.m_refcount)
        __sync_fetch_and_add(other.m_refcount, 1u);
      clear();
      m_ptr = other.m_ptr;
      m_refcount = other.m_refcount;
    }
    return *this;
  }

  /**
   * @brief Move assignment operator. Transfers ownership from another
   * RetainPtr.
   * @param other RetainPtr to move from
   * @return Reference to this
   */
  RetainPtr &operator=(RetainPtr &&other) noexcept {
    if (this != &other) {
      clear();
      m_ptr = other.m_ptr;
      m_refcount = other.m_refcount;
      other.m_ptr = nullptr;
      other.m_refcount = nullptr;
    }
    return *this;
  }

  /**
   * @brief Assign from a raw pointer. Resets previous pointer if any.
   * @param ptr Raw pointer to manage
   * @return Reference to this
   */
  RetainPtr &operator=(T *ptr) {
    if (m_ptr != ptr) {
      clear();
      if (ptr) {
        m_ptr = ptr;
        m_refcount = fk::memory::allocate<uint32_t>();
        if (m_refcount) {
          *m_refcount = 1;
        } else {
          m_ptr = nullptr;
        }
      }
    }
    return *this;
  }

  // --- Pointer management ---

  /**
   * @brief Adopt a raw pointer without creating a new reference count.
   * @param ptr Pointer to adopt
   */
  void adopt(T *ptr) {
    clear();
    m_ptr = ptr;
    if (ptr) {
      m_refcount = fk::memory::allocate<uint32_t>();
      if (m_refcount) {
        *m_refcount = 1;
      } else {
        m_ptr = nullptr;
      }
    } else {
      m_refcount = nullptr;
    }
  }

  /**
   * @brief Reset the pointer, optionally to a new one.
   * @param ptr New pointer (optional)
   */
  void reset(T *ptr = nullptr) {
    clear();
    if (ptr) {
      m_ptr = ptr;
      m_refcount = fk::memory::allocate<uint32_t>();
      if (m_refcount) {
        *m_refcount = 1;
      } else {
        m_ptr = nullptr;
      }
    } else {
      m_refcount = nullptr;
    }
  }

  /**
   * @brief Release ownership without deleting the object.
   * @return Raw pointer
   */
  T *leakRef() {
    T *tmp = m_ptr;
    m_ptr = nullptr;
    m_refcount = nullptr;
    return tmp;
  }

  /**
   * @brief Get the raw pointer.
   * @return Managed pointer
   */
  T *get() const { return m_ptr; }

  T *operator->() const { return m_ptr; }

  T &operator*() const { return *m_ptr; }

  /**
   * @brief Check if the RetainPtr is non-null.
   * @return True if pointer is valid
   */
  explicit operator bool() const { return m_ptr != nullptr; }

  /**
   * @brief Swap contents with another RetainPtr.
   * @param other RetainPtr to swap with
   */
  void swap(RetainPtr &other) {
    T *tmp_ptr = m_ptr;
    uint32_t *tmp_ref = m_refcount;
    m_ptr = other.m_ptr;
    m_refcount = other.m_refcount;
    other.m_ptr = tmp_ptr;
    other.m_refcount = tmp_ref;
  }

  /**
   * @brief Compare two RetainPtr for equality.
   * @param other Other RetainPtr
   * @return True if both point to the same object
   */
  bool operator==(const RetainPtr &other) const { return m_ptr == other.m_ptr; }

  /**
   * @brief Compare two RetainPtr for inequality.
   * @param other Other RetainPtr
   * @return True if they point to different objects
   */
  bool operator!=(const RetainPtr &other) const { return m_ptr != other.m_ptr; }

private:
  T *m_ptr;             ///< Managed pointer
  uint32_t *m_refcount; ///< Internal reference counter (atomic)

  /**
   * @brief Clear the managed pointer. Decrements refcount and deletes
   * the object if it reaches zero.
   */
  void clear() {
    if (!m_refcount) {
      m_ptr = nullptr;
      return;
    }
    
    uint32_t old_count = __sync_fetch_and_sub(m_refcount, 1u);
    if (old_count == 0) {
      fk::algorithms::kerror("RetainPtr", "Refcount already zero! possible double clear at %p", m_ptr);
      m_ptr = nullptr;
      m_refcount = nullptr;
      return;
    }

    uint32_t new_count = old_count - 1;
    if (new_count == 0) {
      if (m_ptr) {
        // Validate pointer before virtual destructor call
        if (reinterpret_cast<uintptr_t>(m_ptr) < 0x1000) {
          fk::algorithms::kerror("RetainPtr", "Invalid pointer detected: %p", m_ptr);
        } else {
          m_ptr->~T();
          fk::memory::deallocate(m_ptr);
        }
      }
      fk::memory::deallocate(m_refcount);
    }
    m_ptr = nullptr;
    m_refcount = nullptr;
  }
};

/**
 * @brief Adopt an existing raw pointer without retaining it.
 * @tparam T Type of pointer
 * @param ptr Pointer to adopt
 * @return RetainPtr managing the pointer
 */
template <typename T> inline RetainPtr<T> adopt_retain(T *ptr) {
  return RetainPtr<T>(RetainPtr<T>::Adopt::Yes, ptr);
}

/**
 * @brief Construct a new managed object with make_retain.
 *
 * Allocates memory in a freestanding way and constructs the object.
 *
 * @tparam T Type of object
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return fk::core::Result containing RetainPtr<T> on success,
 *         or OutOfMemory error on failure
 */
template <typename T, typename... Args>
inline fk::core::Result<RetainPtr<T>, fk::core::Error>
make_retain(Args &&...args) {
  T *obj = fk::memory::allocate<T>();
  if (!obj) {
    return fk::core::Error::OutOfMemory;
  }
  new (obj) T(static_cast<Args &&>(args)...); // placement new freestanding
  return fk::core::Result<RetainPtr<T>>(adopt_retain(obj));
}

} // namespace memory
} // namespace fk
