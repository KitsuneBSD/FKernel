#pragma once

#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace functional {

template <typename> class Function;

template <typename R, typename... Args> class Function<R(Args...)> {
private:
  class CallableBase {
  public:
    virtual ~CallableBase() = default;
    virtual R invoke(Args... args) = 0;
    virtual OwnPtr<CallableBase> clone() const = 0;
  };

  template <typename F> class Callable : public CallableBase {
  public:
    Callable(F f) : m_callable(fk::move(f)) {}

    R invoke(Args... args) override { return m_callable(fk::move(args)...); }

    OwnPtr<CallableBase> clone() const override {
      return make_own<Callable<F>>(m_callable);
    }

  private:
    F m_callable;
  };

  OwnPtr<CallableBase> m_callable;

public:
  Function() = default;

  template <typename F>
  Function(F f) : m_callable(make_own<Callable<F>>(fk::move(f))) {}

  Function(const Function &other) {
    if (other.m_callable) {
      m_callable = other.m_callable->clone();
    }
  }

  Function(Function &&other) noexcept = default;

  Function &operator=(const Function &other) {
    if (this != &other) {
      if (other.m_callable) {
        m_callable = other.m_callable->clone();
      } else {
        m_callable.clear();
      }
    }
    return *this;
  }

  Function &operator=(Function &&other) noexcept = default;

  R operator()(Args... args) const {
    return m_callable->invoke(fk::move(args)...);
  }

  explicit operator bool() const { return m_callable != nullptr; }
};

} // namespace functional
} // namespace fk