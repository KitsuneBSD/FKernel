#pragma once

#include <LibFK/Core/Error.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace core {

template <typename T, typename E = Error>
class Result {
public:
  // Constructors
  // When constructing with a value, initialize m_value and ensure m_error is default (no error).
  Result(const T &value) : m_value(value), m_error() {}
  Result(T &&value) : m_value(fk::move(value)), m_error() {}
  // When constructing with an error, initialize m_error and ensure m_value is empty.
  Result(E error) : m_value(), m_error(error) {}

  // Destructor
  ~Result() = default;

  // Copy and Move
  Result(const Result &other) = default;
  Result &operator=(const Result &other) = default;
  Result(Result &&other) = default;
  Result &operator=(Result &&other) = default;

  // Observers
  // is_ok() is true if m_value has a value (i.e., no error).
  bool is_ok() const { return m_value.has_value(); }
  // is_error() is true if m_value does not have a value.
  bool is_error() const { return !m_value.has_value(); }

  const T &value() const {
    // ASSERT(is_ok()); // Consider adding assertions for debug builds
    return m_value.value();
  }

  T &value() {
    // ASSERT(is_ok()); // Consider adding assertions for debug builds
    return m_value.value();
  }

  E error() const {
    // ASSERT(is_error()); // Consider adding assertions for debug builds
    return m_error;
  }

private:
  // m_value stores the actual value if the operation was successful.
  // Its internal has_value_ flag indicates success or failure.
  optional<T> m_value;
  // m_error stores the error code if m_value is empty.
  E m_error;
};

// Specialization for void return type
template <typename E>
class Result<void, E> {
public:
  // Constructor for success (no value, no error)
  Result() : m_error(Error::None) {}
  // Constructor for error
  Result(E error) : m_error(error) {}

  ~Result() = default;

  Result(const Result &other) = default;
  Result &operator=(const Result &other) = default;
  Result(Result &&other) = default;
  Result &operator=(Result &&other) = default;

  // is_ok() is true if m_error is Error::None.
  bool is_ok() const { return m_error == Error::None; }
  // is_error() is true if m_error is not Error::None.
  bool is_error() const { return m_error != Error::None; }

  // No value() method for void specialization.
  void value() const {
    // ASSERT(is_ok()); // Consider adding assertions for debug builds
  }

  E error() const {
    // ASSERT(is_error()); // Consider adding assertions for debug builds
    return m_error;
  }

private:
  // Stores the error code. If Error::None, it indicates success.
  E m_error;
};

} // namespace core
} // namespace fk