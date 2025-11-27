#pragma once

#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>
#include <LibFK/Traits/type_traits.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace memory {

/**
 * @brief OwnPtr: exclusive ownership smart pointer, similar to std::unique_ptr.
 *
 * OwnPtr exclusively owns a dynamically allocated object and deletes it
 * when the pointer goes out of scope. Supports move semantics but not copy
 * semantics.
 *
 * @tparam T Type of object being owned.
 */
template <typename T> class OwnPtr {
public:
  /** @brief Default constructor: no object owned */
  OwnPtr() : m_ptr(nullptr) {
    fk::algorithms::kdebug("OWN PTR", " default constructed with nullptr");
  }

  /** @brief Construct OwnPtr from raw pointer */
  explicit OwnPtr(T *ptr) : m_ptr(ptr) {
    if (m_ptr)
      fk::algorithms::kdebug("OWN PTR", " constructed, ptr=%p", m_ptr);
    else
      fk::algorithms::kwarn("OWN PTR", " constructed with nullptr");
  }

  /** @brief Move constructor */
  OwnPtr(OwnPtr &&other) : m_ptr(other.leakPtr()) {
    fk::algorithms::kdebug("OWN PTR", " move constructed, ptr=%p", m_ptr);
  }

  /** @brief Move constructor for convertible types */
  template <typename U>
  OwnPtr(OwnPtr<U> &&other) : m_ptr(static_cast<T *>(other.leakPtr())) {
    fk::algorithms::kdebug("OWN PTR", " templated move constructed, ptr=%p",
                           m_ptr);
  }

  /** @brief Destructor */
  ~OwnPtr() {
    if (m_ptr)
      fk::algorithms::kdebug("OWN PTR", " destroying ptr=%p", m_ptr);
    clear();
  }

  /** @brief Construct OwnPtr from nullptr */
  OwnPtr(nullptr_t) : m_ptr(nullptr) {
    fk::algorithms::kdebug("OWN PTR", " constructed from nullptr literal");
  }

  /** @brief Move assignment */
  OwnPtr &operator=(OwnPtr &&other) {
    if (this != &other) {
      clear();
      m_ptr = other.leakPtr();
      fk::algorithms::kdebug("OWN PTR", " move assigned, ptr=%p", m_ptr);
    }
    return *this;
  }

  /** @brief Move assignment for convertible types */
  template <typename U> OwnPtr &operator=(OwnPtr<U> &&other) {
    if (reinterpret_cast<void *>(this) != reinterpret_cast<void *>(&other)) {
      clear();
      m_ptr = static_cast<T *>(other.leakPtr());
      fk::algorithms::kdebug("OWN PTR", " templated move assigned, ptr=%p",
                             m_ptr);
    }
    return *this;
  }

  /** @brief Assign from raw pointer */
  OwnPtr &operator=(T *ptr) {
    if (m_ptr != ptr) {
      clear();
      m_ptr = ptr;
      fk::algorithms::kdebug("OWN PTR", " assigned raw pointer ptr=%p", m_ptr);
    }
    return *this;
  }

  /** @brief Assign nullptr */
  OwnPtr &operator=(nullptr_t) {
    clear();
    fk::algorithms::kdebug("OWN PTR", " assigned nullptr");
    return *this;
  }

  /** @brief Clear owned pointer */
  void clear() {
    if (m_ptr) {
      fk::algorithms::kdebug("OWN PTR", " clearing ptr=%p", m_ptr);
      delete m_ptr;
      m_ptr = nullptr;
    } else {
      fk::algorithms::kdebug("OWN PTR", " clear called on nullptr");
    }
  }

  /** @brief Relinquish ownership without deleting */
  T *leakPtr() {
    T *leaked = m_ptr;
    m_ptr = nullptr;
    fk::algorithms::kdebug("OWN PTR", " leaked ptr=%p", leaked);
    return leaked;
  }

  /** @brief Access pointer */
  T *ptr() { return m_ptr; }
  const T *ptr() const { return m_ptr; }

  /** @brief Dereference operators */
  T *operator->() { return m_ptr; }
  const T *operator->() const { return m_ptr; }

  T &operator*() { return *m_ptr; }
  const T &operator*() const { return *m_ptr; }

  /** @brief Boolean check */
  operator bool() const { return m_ptr != nullptr; }
  bool operator!() const { return !m_ptr; }

private:
  T *m_ptr; ///< Raw pointer to owned object
};

/**
 * @brief Specialization of OwnPtr for array types.
 *
 * Ensures delete[] is used instead of delete.
 *
 * @tparam T Type of elements in array.
 */
template <typename T> class OwnPtr<T[]> {
public:
  OwnPtr() : m_ptr(nullptr) {
    fk::algorithms::kdebug("OWN PTR", "<T[]> default constructed with nullptr");
  }

  explicit OwnPtr(T *ptr) : m_ptr(ptr) {
    if (m_ptr)
      fk::algorithms::kdebug("OWN PTR", "<T[]> constructed, ptr=%p", m_ptr);
    else
      fk::algorithms::kwarn("OWN PTR", "<T[]> constructed with nullptr");
  }

  OwnPtr(OwnPtr &&other) : m_ptr(other.leakPtr()) {
    fk::algorithms::kdebug("OWN PTR", "<T[]> move constructed, ptr=%p", m_ptr);
  }

  ~OwnPtr() { clear(); }

  OwnPtr(nullptr_t) : m_ptr(nullptr) {
    fk::algorithms::kdebug("OWN PTR", "<T[]> constructed from nullptr literal");
  }

  OwnPtr &operator=(OwnPtr &&other) {
    if (this != &other) {
      clear();
      m_ptr = other.leakPtr();
      fk::algorithms::kdebug("OWN PTR", "<T[]> move assigned, ptr=%p", m_ptr);
    }
    return *this;
  }

  OwnPtr &operator=(T *ptr) {
    if (m_ptr != ptr) {
      clear();
      m_ptr = ptr;
      fk::algorithms::kdebug("OWN PTR", "<T[]> assigned raw pointer ptr=%p",
                             m_ptr);
    }
    return *this;
  }

  OwnPtr &operator=(nullptr_t) {
    clear();
    fk::algorithms::kdebug("OWN PTR", "<T[]> assigned nullptr");
    return *this;
  }

  void clear() {
    if (m_ptr) {
      fk::algorithms::kdebug("OWN PTR", "<T[]> clearing ptr=%p", m_ptr);
      delete[] m_ptr;
      m_ptr = nullptr;
    } else {
      fk::algorithms::kdebug("OWN PTR", "<T[]> clear called on nullptr");
    }
  }

  T *leakPtr() {
    T *leaked = m_ptr;
    m_ptr = nullptr;
    fk::algorithms::kdebug("OWN PTR", "<T[]> leaked ptr=%p", leaked);
    return leaked;
  }

  T *ptr() { return m_ptr; }
  const T *ptr() const { return m_ptr; }

  T &operator[](size_t i) { return m_ptr[i]; }
  const T &operator[](size_t i) const { return m_ptr[i]; }

  operator bool() const { return m_ptr != nullptr; }
  bool operator!() const { return !m_ptr; }

private:
  T *m_ptr; ///< Raw pointer to owned array
};

/**
 * @brief Adopt an existing raw pointer into an OwnPtr.
 *
 * Transfers ownership of a dynamically allocated object or array
 * to an OwnPtr. No RTTI or typeid is used.
 *
 * @tparam T Type of the object or array element
 * @param ptr Raw pointer to adopt
 * @return OwnPtr<T> owning the object
 */
template <typename T,
          typename = fk::traits::enable_if_t<!fk::traits::is_array_v<T>>>
inline OwnPtr<T> adopt_own(T *ptr) {
  if (!ptr) {
    fk::algorithms::kwarn("ADOPT OWN", "Received nullptr for non-array type");
    return OwnPtr<T>(nullptr);
  }
  fk::algorithms::kdebug("ADOPT OWN",
                         "Ownership transferred for non-array type");
  return OwnPtr<T>(ptr);
}

template <typename T,
          typename = fk::traits::enable_if_t<fk::traits::is_array_v<T>>>
inline OwnPtr<T> adopt_own(fk::traits::remove_extent_t<T> *ptr) {
  if (!ptr) {
    fk::algorithms::kwarn("ADOPT OWN", "Received nullptr for array type");
    return OwnPtr<T>(nullptr);
  }
  fk::algorithms::kdebug("ADOPT OWN", "Ownership transferred for array type");
  return OwnPtr<T>(ptr);
}

} // namespace memory
} // namespace fk
