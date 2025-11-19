#pragma once

#include <LibFK/Memory/new.h>
#include <LibFK/Traits/type_traits.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace memory {

/**
 * @brief A simple ownership smart pointer similar to std::unique_ptr.
 *
 * OwnPtr exclusively owns a dynamically allocated object and deletes it
 * when the pointer goes out of scope. Supports move semantics and
 * pointer access, but not copy semantics.
 *
 * @tparam T Type of the object being owned
 */
template <typename T> class OwnPtr {
public:
  /** @brief Default constructor: no object owned */
  OwnPtr() : m_ptr(nullptr) {}

  /** @brief Construct OwnPtr from raw pointer
   *  @param ptr Raw pointer to own
   */
  explicit OwnPtr(T *ptr) : m_ptr(ptr) {}

  /** @brief Move constructor */
  OwnPtr(OwnPtr &&other) : m_ptr(other.leakPtr()) {}

  /** @brief Templated move constructor for convertible types */
  template <typename U>
  OwnPtr(OwnPtr<U> &&other) : m_ptr(static_cast<T *>(other.leakPtr())) {}

  /** @brief Destructor: deletes owned object */
  ~OwnPtr() { clear(); }

  /** @brief Construct OwnPtr from nullptr */
  OwnPtr(nullptr_t) : m_ptr(nullptr) {}

  /** @brief Move assignment operator */
  OwnPtr &operator=(OwnPtr &&other) {
    if (this != &other) {
      clear();
      m_ptr = other.leakPtr();
    }
    return *this;
  }

  /** @brief Templated move assignment operator for convertible types */
  template <typename U> OwnPtr &operator=(OwnPtr<U> &&other) {
    if (reinterpret_cast<void *>(this) != reinterpret_cast<void *>(&other)) {
      clear();
      m_ptr = static_cast<T *>(other.leakPtr());
    }
    return *this;
  }

  /** @brief Assign from raw pointer */
  OwnPtr &operator=(T *ptr) {
    if (m_ptr != ptr)
      clear();
    m_ptr = ptr;
    return *this;
  }

  /** @brief Assign nullptr, releasing owned object */
  OwnPtr &operator=(nullptr_t) {
    clear();
    return *this;
  }

  /** @brief Releases ownership and deletes owned object */
  void clear() {
    if (m_ptr) {
      delete m_ptr;
      m_ptr = nullptr;
    }
  }

  /** @brief Check if pointer is null
   *  @return true if no object is owned
   */
  bool operator!() const { return !m_ptr; }

  /** @brief Relinquish ownership without deleting
   *  @return Pointer that was owned
   */
  T *leakPtr() {
    T *leaked = m_ptr;
    m_ptr = nullptr;
    return leaked;
  }

  /** @brief Access owned pointer */
  T *ptr() { return m_ptr; }
  const T *ptr() const { return m_ptr; }

  /** @brief Dereference operators */
  T *operator->() { return m_ptr; }
  const T *operator->() const { return m_ptr; }

  T &operator*() { return *m_ptr; }
  const T &operator*() const { return *m_ptr; }

  /** @brief Check if pointer is non-null
   *  @return true if an object is owned
   */
  operator bool() const { return m_ptr != nullptr; }

private:
  T *m_ptr; ///< Raw pointer to the owned object
};

/**
 * @brief Specialization of OwnPtr for array types.
 *
 * This specialization ensures that `delete[]` is used for arrays,
 * preventing memory leaks and undefined behavior.
 *
 * @tparam T Type of the elements in the array
 */
template <typename T> class OwnPtr<T[]> {
public:
  /** @brief Default constructor: no object owned */
  OwnPtr() : m_ptr(nullptr) {}

  /** @brief Construct OwnPtr from raw array pointer
   *  @param ptr Raw pointer to array to own
   */
  explicit OwnPtr(T *ptr) : m_ptr(ptr) {}

  /** @brief Move constructor */
  OwnPtr(OwnPtr &&other) : m_ptr(other.leakPtr()) {}

  /** @brief Destructor: deletes owned array */
  ~OwnPtr() { clear(); }

  /** @brief Construct OwnPtr from nullptr */
  OwnPtr(nullptr_t) : m_ptr(nullptr) {}

  /** @brief Move assignment operator */
  OwnPtr &operator=(OwnPtr &&other) {
    if (this != &other) {
      clear();
      m_ptr = other.leakPtr();
    }
    return *this;
  }

  /** @brief Assign from raw array pointer */
  OwnPtr &operator=(T *ptr) {
    if (m_ptr != ptr)
      clear();
    m_ptr = ptr;
    return *this;
  }

  /** @brief Assign nullptr, releasing owned array */
  OwnPtr &operator=(nullptr_t) {
    clear();
    return *this;
  }

  /** @brief Releases ownership and deletes owned array */
  void clear() {
    if (m_ptr) {
      delete[] m_ptr;
      m_ptr = nullptr;
    }
  }

  /** @brief Check if pointer is null
   *  @return true if no object is owned
   */
  bool operator!() const { return !m_ptr; }

  /** @brief Relinquish ownership without deleting
   *  @return Pointer to array that was owned
   */
  T *leakPtr() {
    T *leaked = m_ptr;
    m_ptr = nullptr;
    return leaked;
  }

  /** @brief Access owned array pointer */
  T *ptr() { return m_ptr; }
  const T *ptr() const { return m_ptr; }

  /** @brief Array subscript operator */
  T &operator[](size_t i) { return m_ptr[i]; }
  const T &operator[](size_t i) const { return m_ptr[i]; }

  /** @brief Check if pointer is non-null
   *  @return true if an array is owned
   */
  operator bool() const { return m_ptr != nullptr; }

private:
  T *m_ptr; ///< Raw pointer to the owned array
};

/**
 * @brief Helper function to construct OwnPtr in-place
 *
 * @tparam T Type to construct
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return fk::core::Result containing OwnPtr<T> on success, or OutOfMemory on
 * failure.
 */
template <typename T, typename... Args>
inline fk::core::Result<OwnPtr<T>, fk::core::Error> make_own(Args &&...args) {
  // In a freestanding environment, 'new' might return nullptr on failure.
  T *ptr = new T(static_cast<Args &&>(args)...);
  if (!ptr) { // Check if new returned nullptr
    return fk::core::Error::OutOfMemory;
  }
  return fk::core::Result<OwnPtr<T>>(OwnPtr<T>(ptr));
}

/**
 * @brief Adopt an existing raw pointer into an OwnPtr.
 *
 * Use this when you already have a dynamically allocated object
 * and want to transfer ownership to an OwnPtr.
 *
 * @tparam T Type of object
 * @param ptr Raw pointer to adopt
 * @return OwnPtr<T> owning the object
 */
template <typename T,
          typename = fk::traits::enable_if_t<!fk::traits::is_array_v<T>>>
inline OwnPtr<T> adopt_own(T *ptr) {
  return OwnPtr<T>(ptr);
}

/**
 * @brief Adopt an existing raw array pointer into an OwnPtr<T[]>.
 *
 * Use this when you already have a dynamically allocated array
 * and want to transfer ownership to an OwnPtr<T[]>.
 *
 * @tparam T Type of array elements (e.g., `uint8_t[]`)
 * @param ptr Raw pointer to array to adopt (e.g., `uint8_t*`)
 * @return OwnPtr<T> owning the array
 */
template <typename T,
          typename = fk::traits::enable_if_t<fk::traits::is_array_v<T>>>
inline OwnPtr<T> adopt_own(fk::traits::remove_extent_t<T> *ptr) {
  return OwnPtr<T>(ptr);
}

} // namespace memory
} // namespace fk
