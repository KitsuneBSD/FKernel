#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Memory/new.h>

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
   * @brief Construct optional with a value (move semantics)
   * @param value Value to store
   */
  optional(T &&value) : has_value_(true) {
    new (storage) T(static_cast<T &&>(value));
    fk::algorithms::kdebug("OPTIONAL", "Constructed with move value.");
  }

  /**
   * @brief Construct optional with a value (copy semantics)
   * @param val Value to store
   */
  optional(const T &val) : has_value_(true) {
    new (storage) T(val);
    fk::algorithms::kdebug("OPTIONAL", "Constructed with copy value.");
  }

  /**
   * @brief Copy constructor
   * @param other Other optional to copy
   */
  optional(const optional &other) : has_value_(other.has_value_) {
    if (has_value_)
      new (storage) T(*other.ptr());
    fk::algorithms::kdebug("OPTIONAL",
                           "Constructed with copy optional. Has value: %b",
                           has_value_);
  }

  /** @brief Destructor: destroys stored value if present */
  ~optional() { reset(); }

  /**
   * @brief Move constructor
   * @param other Other optional to move from
   */
  optional(optional &&other) : has_value_(other.has_value_) {
    if (has_value_) {
      new (storage) T(static_cast<T &&>(*other.ptr()));
      other.reset(); // Ensure the other optional is empty
    }
    fk::algorithms::kdebug("OPTIONAL",
                           "Constructed with move optional. Has value: %b",
                           has_value_);
  }

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
    fk::algorithms::kdebug("OPTIONAL", "Copy assigned. Has value: %b",
                           has_value_);
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
  T &value() {
    if (!has_value_) {
      fk::algorithms::kdebug("OPTIONAL",
          "Attempted to access value of empty optional (non-const).");
    }
    return *ptr();
  }

  /**
   * @brief Access the stored value (const version)
   * @return Const reference to stored value
   */
  const T &value() const {
    if (!has_value_) {
      fk::algorithms::kdebug("OPTIONAL", "Attempted to access value of empty optional (const).");
    }
    return *ptr();
  }

  /**
   * @brief Reset the optional, destroying stored value if present
   */
  void reset() {
    if (has_value_) {
      ptr()->~T();
      has_value_ = false;
      fk::algorithms::kdebug("OPTIONAL", "Value destroyed during reset.");
    }
  }
};

} // namespace memory
} // namespace fk
