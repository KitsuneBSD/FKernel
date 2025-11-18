#pragma once

#include <LibFK/Core/Error.h>
#include <LibFK/Core/Result.h>
#include <LibFK/Types/types.h>
#include <LibFK/new.h>

namespace fk {
namespace memory {

/**
 * @brief A smart pointer with retain/release semantics.
 *
 * This pointer manages objects that implement retain() and release().
 * It automatically retains on copy and releases on destruction.
 *
 * @tparam T Type of object being managed. Must provide `retain()` and
 * `release()`.
 */
template <typename T> class RetainPtr {
public:
  /// Tag type to indicate adoption of a pointer without retaining
  enum class Adopt { Yes };

  /**
   * @brief Default constructor. Constructs an empty RetainPtr.
   */
  RetainPtr() : m_ptr(nullptr) {}

  /**
   * @brief Construct RetainPtr managing the given pointer (retains it).
   * @param ptr Pointer to manage
   */
  explicit RetainPtr(T *ptr) : m_ptr(ptr) {
    if (m_ptr)
      m_ptr->retain();
  }

  /**
   * @brief Adopt an already retained pointer without incrementing reference.
   * @param Adopt Tag type (Adopt::Yes)
   * @param ptr Pointer to adopt
   */
  RetainPtr(Adopt, T *ptr) : m_ptr(ptr) {}

  /**
   * @brief Copy constructor. Retains the underlying pointer.
   * @param other Another RetainPtr to copy
   */
  RetainPtr(const RetainPtr &other) : m_ptr(other.m_ptr) {
    if (m_ptr)
      m_ptr->retain();
  }

  /**
   * @brief Copy constructor from compatible type.
   * @tparam U Type of other RetainPtr
   * @param other Another RetainPtr
   */
  template <typename U>
  RetainPtr(const RetainPtr<U> &other) : m_ptr(static_cast<T *>(other.get())) {
    if (m_ptr)
      m_ptr->retain();
  }

  /**
   * @brief Move constructor.
   * @param other RetainPtr to move from
   */
  RetainPtr(RetainPtr &&other) noexcept : m_ptr(other.leakRef()) {}

  /**
   * @brief Move constructor from compatible type.
   * @tparam U Type of other RetainPtr
   * @param other RetainPtr to move from
   */
  template <typename U>
  RetainPtr(RetainPtr<U> &&other) noexcept
      : m_ptr(static_cast<T *>(other.leakRef())) {}

  /**
   * @brief Destructor. Releases the managed pointer.
   */
  ~RetainPtr() { clear(); }

  // --- Assignment operators ---

  RetainPtr &operator=(const RetainPtr &other) {
    if (this != &other) {
      if (other.m_ptr)
        other.m_ptr->retain();
      clear();
      m_ptr = other.m_ptr;
    }
    return *this;
  }

  template <typename U> RetainPtr &operator=(const RetainPtr<U> &other) {
    T *new_ptr = static_cast<T *>(other.get());
    if (new_ptr)
      new_ptr->retain();
    clear();
    m_ptr = new_ptr;
    return *this;
  }

  RetainPtr &operator=(RetainPtr &&other) noexcept {
    if (this != &other) {
      clear();
      m_ptr = other.leakRef();
    }
    return *this;
  }

  template <typename U> RetainPtr &operator=(RetainPtr<U> &&other) noexcept {
    clear();
    m_ptr = static_cast<T *>(other.leakRef());
    return *this;
  }

  RetainPtr &operator=(T *ptr) {
    if (m_ptr != ptr) {
      if (ptr)
        ptr->retain();
      clear();
      m_ptr = ptr;
    }
    return *this;
  }

  // --- Pointer management ---

  /**
   * @brief Adopt a pointer without retaining it.
   * @param ptr Pointer to adopt
   */
  void adopt(T *ptr) {
    clear();
    m_ptr = ptr;
  }

  /**
   * @brief Release current pointer and optionally set a new one.
   * @param ptr New pointer to manage (optional)
   */
  void reset(T *ptr = nullptr) {
    if (ptr != m_ptr) {
      if (ptr)
        ptr->retain();
      clear();
      m_ptr = ptr;
    }
  }

  /**
   * @brief Release ownership without releasing the reference.
   * @return The raw pointer
   */
  T *leakRef() {
    T *tmp = m_ptr;
    m_ptr = nullptr;
    return tmp;
  }

  /**
   * @brief Get the raw pointer.
   * @return Pointer being managed
   */
  T *get() { return m_ptr; }
  const T *get() const { return m_ptr; }

  T *operator->() { return m_ptr; }
  const T *operator->() const { return m_ptr; }

  T &operator*() { return *m_ptr; }
  const T &operator*() const { return *m_ptr; }

  explicit operator bool() const { return m_ptr != nullptr; }

  /**
   * @brief Swap contents with another RetainPtr.
   * @param other RetainPtr to swap with
   */
  void swap(RetainPtr &other) {
    T *tmp = m_ptr;
    m_ptr = other.m_ptr;
    other.m_ptr = tmp;
  }

  bool operator==(const RetainPtr &other) const { return m_ptr == other.m_ptr; }
  bool operator!=(const RetainPtr &other) const { return m_ptr != other.m_ptr; }

private:
  T *m_ptr;

  void clear() {
    if (m_ptr) {
      m_ptr->release();
      m_ptr = nullptr;
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
  return RetainPtr<T>(RetainPtr<T>::Adopt::Yes, ptr);
}

/**
 * @brief Construct a new managed object with make_retain.
 * @tparam T Type of object
 * @tparam Args Constructor argument types
 * @param args Constructor arguments
 * @return fk::core::Result containing RetainPtr<T> on success, or OutOfMemory
 * on failure.
 */
template <typename T, typename... Args>
inline fk::core::Result<RetainPtr<T>, fk::core::Error>
make_retain(Args &&...args) {
  // In a freestanding environment, 'new' might return nullptr on failure.
  T *obj = new T(static_cast<Args &&>(args)...);
  if (!obj) { // Check if new returned nullptr
    return fk::core::Error::OutOfMemory;
  }
  return fk::core::Result<RetainPtr<T>>(adopt_retain(obj));
}

} // namespace memory
} // namespace fk
