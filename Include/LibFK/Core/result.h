#pragma once

#include <LibFK/Core/assertions.h>
#include <LibFK/Core/error.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Memory/optional.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace core {

template <typename T, typename E = Error>
class Result {
public:
  Result(const T &value) : m_value(value), m_error() {}
  Result(T &&value) : m_value(static_cast<T&&>(value)), m_error() {}
  Result(E error) : m_value(), m_error(error) {}

  ~Result() = default;
  Result(const Result &other) = default;
  Result &operator=(const Result &other) = default;
  Result(Result &&other) = default;
  Result &operator=(Result &&other) = default;

  bool is_ok() const { return m_value.has_value(); }
  bool is_error() const { return !m_value.has_value(); }
  explicit operator bool() const { return is_ok(); }

  const T &value() const {
    if (is_error()) __builtin_trap();
    return m_value.value();
  }

  T &value() {
    if (is_error()) __builtin_trap();
    return m_value.value();
  }

  E error() const {
    if (is_ok()) __builtin_trap();
    return m_error;
  }

private:
  // m_value stores the actual value if the operation was successful.
  // Its internal has_value_ flag indicates success or failure.
  fk::memory::optional<T> m_value;
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

  // Allow using in if statements: if (res) { ... }
  explicit operator bool() const { return is_ok(); }

  void value() const {
    if (is_error())
      fk::algorithms::kwarn("RESULT", "value() called on error result (void)");
  }

  E error() const {
    if (is_ok())
      fk::algorithms::kwarn("RESULT", "error() called on ok result (void)");
    return m_error;
  }

private:
  // Stores the error code. If Error::None, it indicates success.
  E m_error;
};

} // namespace core
} // namespace fk

#define TRY(expression)                                                        \
  ({                                                                           \
    auto _result = (expression);                                               \
    if (_result.is_error())                                                    \
      return _result.error();                                                  \
    _result.value();                                                           \
  })
