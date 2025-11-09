#pragma once

#include <LibFK/Types/types.h>
#include <LibFK/new.h>

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
 * @brief Helper function to construct OwnPtr in-place
 *
 * @tparam T Type to construct
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return OwnPtr<T> owning the new object
 */
template <typename T, typename... Args>
inline OwnPtr<T> make_own(Args &&...args) {
  return OwnPtr<T>(new T(static_cast<Args &&>(args)...));
}
