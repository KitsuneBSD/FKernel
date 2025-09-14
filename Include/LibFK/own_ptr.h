#pragma once

#include <LibC/stddef.h>
#include <LibFK/new.h>

template <typename T> class OwnPtr {
public:
  OwnPtr() : m_ptr(nullptr) {}

  explicit OwnPtr(T *ptr) : m_ptr(ptr) {}

  OwnPtr(OwnPtr &&other) : m_ptr(other.leakPtr()) {}

  template <typename U>
  OwnPtr(OwnPtr<U> &&other) : m_ptr(static_cast<T *>(other.leakPtr())) {}

  ~OwnPtr() { clear(); }

  OwnPtr(nullptr_t) : m_ptr(nullptr) {}

  OwnPtr &operator=(OwnPtr &&other) {
    if (this != &other) {
      clear();
      m_ptr = other.leakPtr();
    }
    return *this;
  }

  template <typename U> OwnPtr &operator=(OwnPtr<U> &&other) {
    if (reinterpret_cast<void *>(this) != reinterpret_cast<void *>(&other)) {
      clear();
      m_ptr = static_cast<T *>(other.leakPtr());
    }
    return *this;
  }

  OwnPtr &operator=(T *ptr) {
    if (m_ptr != ptr)
      clear();
    m_ptr = ptr;
    return *this;
  }

  OwnPtr &operator=(nullptr_t) {
    clear();
    return *this;
  }

  void clear() {
    if (m_ptr) {
      delete m_ptr;
      m_ptr = nullptr;
    }
  }

  bool operator!() const { return !m_ptr; }

  T *leakPtr() {
    T *leaked = m_ptr;
    m_ptr = nullptr;
    return leaked;
  }

  T *ptr() { return m_ptr; }
  const T *ptr() const { return m_ptr; }

  T *operator->() { return m_ptr; }
  const T *operator->() const { return m_ptr; }

  T &operator*() { return *m_ptr; }
  const T &operator*() const { return *m_ptr; }

  operator bool() const { return m_ptr != nullptr; }

private:
  T *m_ptr;
};

template <typename T, typename... Args>
inline OwnPtr<T> make_own(Args &&...args) {
  return OwnPtr<T>(new T(static_cast<Args &&>(args)...));
}
