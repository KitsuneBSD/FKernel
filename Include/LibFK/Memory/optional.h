#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h>
#include <LibFK/new.h>

namespace fk {
namespace memory {

/**
 * @brief Minimal freestanding implementation of std::optional.
 *
 * Stores an object of type T in-place without dynamic allocation.
 * Provides basic copy semantics and manual reset.
 *
 * @tparam T Type of the stored value
 */
template <typename T> class optional {
private:
  bool has_value_{false}; ///< Indicates whether a value is stored
  alignas(T) unsigned char storage[sizeof(T)]; ///< Raw storage for T

  /**
   * @brief Get a pointer to the stored object.
   * @return Pointer to T
   */
  T *ptr() { return reinterpret_cast<T *>(storage); }

  /**
   * @brief Get a const pointer to the stored object.
   * @return Const pointer to T
   */
  const T *ptr() const { return reinterpret_cast<const T *>(storage); }

public:
  /** @brief Default constructor: no value stored */
  constexpr optional() = default;

  /**
   * @brief Construct optional with a value
   * @param value Value to store
   */
  optional(const T &value) : has_value_(true) { new (storage) T(value); }

  /**
   * @brief Copy constructor
   * @param other Other optional to copy
   */
  optional(const optional &other) : has_value_(other.has_value_) {
    if (has_value_)
      new (storage) T(*other.ptr());
  }

  /** @brief Destructor: destroys stored value if present */
  ~optional() { reset(); }

  /**
   * @brief Copy assignment
   * @param other Other optional
   * @return Reference to this
   */
  optional &operator=(const optional &other) {
    if (this != &other) {
      reset();
      if (other.has_value_) {
        new (storage) T(*other.ptr());
        has_value_ = true;
      }
    }
    return *this;
  }

  /**
   * @brief Check if a value is stored
   * @return true if a value is stored, false otherwise
   */
  constexpr bool has_value() const { return has_value_; }

  /**
   * @brief Access the stored value
   * @return Reference to stored value
   */
  T &value() { return *ptr(); }

  /**
   * @brief Access the stored value (const version)
   * @return Const reference to stored value
   */
  const T &value() const { return *ptr(); }

  /**
   * @brief Reset the optional, destroying stored value if present
   */
  void reset() {
    if (has_value_) {
      ptr()->~T();
      has_value_ = false;
    }
  }
};

} // namespace memory
} // namespace fk