#pragma once

#include <LibFK/Traits/type_traits.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace functional {

template <typename... Ts> class Tuple;

// Base case: empty tuple
template <> class Tuple<> {};

// Recursive case
template <typename T, typename... Ts>
class Tuple<T, Ts...> : public Tuple<Ts...> {
public:
  using Base = Tuple<Ts...>;

  Tuple(const T &t, const Ts &...ts) : Base(ts...), m_head(t) {}

  T &head() { return m_head; }
  const T &head() const { return m_head; }

  Base &tail() { return *this; }
  const Base &tail() const { return *this; }

protected:
  T m_head;
};

// Helper to get the Nth element type
template <size_t N, typename T> struct tuple_element;

template <size_t N, typename T, typename... Ts>
struct tuple_element<N, Tuple<T, Ts...>> {
  using type = typename tuple_element<N - 1, Tuple<Ts...>>::type;
};

template <typename T, typename... Ts>
struct tuple_element<0, Tuple<T, Ts...>> {
  using type = T;
};

template <size_t N, typename T>
using tuple_element_t = typename tuple_element<N, T>::type;

// Helper to get the Nth element value
template <size_t N, typename... Ts> auto &get(Tuple<Ts...> &t) {
  if constexpr (N == 0) {
    return t.head();
  } else {
    return get<N - 1>(t.tail());
  }
}

template <size_t N, typename... Ts> const auto &get(const Tuple<Ts...> &t) {
  if constexpr (N == 0) {
    return t.head();
  } else {
    return get<N - 1>(t.tail());
  }
}

template <typename... Types>
constexpr Tuple<Types...> make_tuple(const Types &...args) {
  return Tuple<Types...>(args...);
}

} // namespace functional
} // namespace fk
