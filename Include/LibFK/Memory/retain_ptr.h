#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
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
  RetainPtr() : m_ptr(nullptr), m_refcount(nullptr) {
    fk::algorithms::kdebug("RETAIN PTR", "Default constructed empty");
  }

  /**
   * @brief Constructs RetainPtr managing a given pointer.
   * @param ptr Pointer to manage
   */
  explicit RetainPtr(T *ptr) : m_ptr(ptr) {
    if (m_ptr) {
      m_refcount = fk::memory::allocate<size_t>();
      if (m_refcount) {
        *m_refcount = 1;
        fk::algorithms::kdebug("RETAIN PTR", "New object retained %p",
                               (void *)m_ptr);
      } else {
        fk::algorithms::kdebug("RETAIN PTR", "Failed to allocate refcount");
        m_ptr = nullptr;
      }
    } else {
      m_refcount = nullptr;
    }
  }

  /**
   * @brief Adopt a pointer without creating a new reference count.
   * @param Adopt Tag (Adopt::Yes)
   * @param ptr Pointer to adopt
   */
  RetainPtr(Adopt, T *ptr) : m_ptr(ptr) {
    if (m_ptr) {
      m_refcount = fk::memory::allocate<size_t>();
      if (m_refcount) {
        *m_refcount = 1;
        fk::algorithms::kdebug("RETAIN PTR", "Adopted pointer %p", (void *)ptr);
      } else {
        fk::algorithms::kdebug("RETAIN PTR",
                              "Failed to allocate refcount for adopt");
        m_ptr = nullptr;
      }
    } else {
      m_refcount = nullptr;
    }
  }

  /**
   * @brief Copy constructor for derived types.
   * Allows RetainPtr<Derived> -> RetainPtr<Base> conversion.
   */
  template <typename U,
            typename = fk::traits::enable_if_t<fk::traits::is_base_of_v<U, T>>>
  RetainPtr(const RetainPtr<U> &other) noexcept
      : m_ptr(other.get()), m_refcount(other.getRefCount()) {
    if (m_refcount)
      ++(*m_refcount);
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
  template <typename U> size_t *getRefCount() const { return m_refcount; }

  /**
   * @brief Copy constructor. Increments the reference count.
   * @param other Another RetainPtr to copy
   */
  RetainPtr(const RetainPtr &other)
      : m_ptr(other.m_ptr), m_refcount(other.m_refcount) {
    if (m_refcount)
      ++(*m_refcount);
    fk::algorithms::kdebug("RETAIN PTR", "copy retained %p", (void *)m_ptr);
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
        ++(*other.m_refcount);
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
        m_refcount = fk::memory::allocate<size_t>();
        if (m_refcount) {
          *m_refcount = 1;
          fk::algorithms::kdebug("RETAIN PTR", "raw pointer assigned ",
                                 (void *)ptr);
        } else {
                  fk::algorithms::kdebug("RETAIN PTR",
                                         "Failed to allocate refcount for raw ptr");          m_ptr = nullptr;
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
      m_refcount = fk::memory::allocate<size_t>();
      if (m_refcount) {
        *m_refcount = 1;
        fk::algorithms::kdebug("RETAIN PTR", "adopt called %p", (void *)ptr);
      } else {
        fk::algorithms::kdebug("RETAIN PTR",
                              "Failed to allocate refcount for adopt()");
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
      m_refcount = fk::memory::allocate<size_t>();
      if (m_refcount) {
        *m_refcount = 1;
        fk::algorithms::kdebug("RETAIN PTR", "reset to new pointer %p",
                               (void *)ptr);
      } else {
        fk::algorithms::kdebug("RETAIN PTR",
                              "Failed to allocate refcount for reset()");
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
    fk::algorithms::kdebug("RETAIN PTR", "leakRef %p", (void *)tmp);
    return tmp;
  }

  /**
   * @brief Get the raw pointer.
   * @return Managed pointer
   */
  T *get() { return m_ptr; }

  /**
   * @brief Get the raw pointer (const version).
   * @return Managed pointer
   */
  const T *get() const { return m_ptr; }

  T *operator->() { return m_ptr; }
  const T *operator->() const { return m_ptr; }

  T &operator*() { return *m_ptr; }
  const T &operator*() const { return *m_ptr; }

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
    size_t *tmp_ref = m_refcount;
    m_ptr = other.m_ptr;
    m_refcount = other.m_refcount;
    other.m_ptr = tmp_ptr;
    other.m_refcount = tmp_ref;
    fk::algorithms::kdebug("RETAIN PTR", "Swap performed");
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
  T *m_ptr;           ///< Managed pointer
  size_t *m_refcount; ///< Internal reference counter

  /**
   * @brief Clear the managed pointer. Decrements refcount and deletes
   * the object if it reaches zero.
   */
  void clear() {
    if (m_refcount) {
      --(*m_refcount);
      fk::algorithms::kdebug("RETAIN PTR",
                             "Decrement refcount on %p refcount=%p",
                             (void *)m_ptr, (void *)*m_refcount);
      if (*m_refcount == 0) {
        fk::memory::deallocate(m_ptr);
        fk::memory::deallocate(m_refcount);
        fk::algorithms::kdebug("RETAIN PTR", "Object deleted a %p",
                               (void *)m_ptr);
      }
      m_ptr = nullptr;
      m_refcount = nullptr;
    }
  }
};

/**
 * @brief Adopt an existing raw pointer without retaining it.
 * @tparam T Type of pointer
 * @param ptr Pointer to adopt
 * @return RetainPtr managing the pointer
 */
template <typename T> inline RetainPtr<T> adopt_retain(T *ptr) {
  fk::algorithms::kdebug("RETAIN PTR", "Adopt Retain called for %p",
                         (void *)ptr);
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
    fk::algorithms::kdebug("RETAIN PTR", "Make retain failed: Out of Memory");
    return fk::core::Error::OutOfMemory;
  }
  new (obj) T(static_cast<Args &&>(args)...); // placement new freestanding
  fk::algorithms::kdebug("RETAIN PTR", "Make retain succeeded: %p",
                         (void *)obj);
  return fk::core::Result<RetainPtr<T>>(adopt_retain(obj));
}

} // namespace memory
} // namespace fk
