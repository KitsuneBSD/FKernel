#pragma once

#include <LibC/stddef.h>
#include <LibFK/new.h>

template <typename T> class RetainPtr {
public:
  enum class Adopt { Yes };

  RetainPtr() : m_ptr(nullptr) {}

  explicit RetainPtr(T *ptr) : m_ptr(ptr) {
    if (m_ptr)
      m_ptr->retain();
  }

  RetainPtr(Adopt, T *ptr) : m_ptr(ptr) {}

  RetainPtr(const RetainPtr &other) : m_ptr(other.m_ptr) {
    if (m_ptr)
      m_ptr->retain();
  }

  template <typename U>
  RetainPtr(const RetainPtr<U> &other) : m_ptr(static_cast<T *>(other.get())) {
    if (m_ptr)
      m_ptr->retain();
  }

  RetainPtr(RetainPtr &&other) noexcept : m_ptr(other.leakRef()) {}

  template <typename U>
  RetainPtr(RetainPtr<U> &&other) noexcept
      : m_ptr(static_cast<T *>(other.leakRef())) {}

  ~RetainPtr() { clear(); }

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
    if (m_ptr == ptr)
      return *this;
    if (ptr)
      ptr->retain();
    clear();
    m_ptr = ptr;
    return *this;
  }

  void adopt(T *ptr) {
    clear();
    m_ptr = ptr;
  }

  void clear() {
    if (m_ptr) {
      m_ptr->release();
      m_ptr = nullptr;
    }
  }

  void reset(T *ptr = nullptr) {
    if (ptr == m_ptr)
      return;
    if (ptr)
      ptr->retain();
    clear();
    m_ptr = ptr;
  }

  T *leakRef() {
    T *tmp = m_ptr;
    m_ptr = nullptr;
    return tmp;
  }

  T *get() { return m_ptr; }
  const T *get() const { return m_ptr; }

  T *operator->() { return m_ptr; }
  const T *operator->() const { return m_ptr; }

  T &operator*() { return *m_ptr; }
  const T &operator*() const { return *m_ptr; }

  explicit operator bool() const { return m_ptr != nullptr; }

  void swap(RetainPtr &other) {
    T *tmp = m_ptr;
    m_ptr = other.m_ptr;
    other.m_ptr = tmp;
  }

  bool operator==(const RetainPtr &other) const { return m_ptr == other.m_ptr; }
  bool operator!=(const RetainPtr &other) const { return m_ptr != other.m_ptr; }

private:
  T *m_ptr;
};

template <typename T> inline RetainPtr<T> adopt_retain(T *ptr) {
  return RetainPtr<T>(typename RetainPtr<T>::Adopt::Yes, ptr);
}

template <typename T, typename... Args>
inline RetainPtr<T> make_retain(Args &&...args) {
  return RetainPtr<T>(new T(static_cast<Args &&>(args)...));
}
