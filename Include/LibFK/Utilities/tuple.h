#pragma once

#include <LibC/stddef.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace utilities {

namespace detail {

template <size_t Index, typename T>
struct TupleLeaf {
  T value;
  TupleLeaf() = default;
  explicit TupleLeaf(T v) : value(fk::types::move(v)) {}
};

template <size_t Index, typename... Ts>
struct TupleImpl {};

template <size_t Index, typename Head, typename... Tail>
struct TupleImpl<Index, Head, Tail...>
    : TupleLeaf<Index, Head>
    , TupleImpl<Index + 1, Tail...> {
  TupleImpl() = default;
  TupleImpl(Head h, Tail... t)
      : TupleLeaf<Index, Head>(fk::types::move(h))
      , TupleImpl<Index + 1, Tail...>(fk::types::move(t)...) {}
};

} // namespace detail

template <typename... Ts>
struct Tuple : detail::TupleImpl<0, Ts...> {
  Tuple() = default;
  explicit Tuple(Ts... args)
      : detail::TupleImpl<0, Ts...>(fk::types::move(args)...) {}
  Tuple(const Tuple&) = default;
  Tuple(Tuple&&) = default;
  Tuple& operator=(const Tuple&) = default;
  Tuple& operator=(Tuple&&) = default;
};

// tuple_size
template <typename T>
struct tuple_size;

template <typename... Ts>
struct tuple_size<Tuple<Ts...>> {
  static constexpr size_t value = sizeof...(Ts);
};

template <typename T>
inline constexpr size_t tuple_size_v = tuple_size<T>::value;

// tuple_element
template <size_t N, typename T>
struct tuple_element;

template <typename Head, typename... Tail>
struct tuple_element<0, Tuple<Head, Tail...>> {
  using type = Head;
};

template <size_t N, typename Head, typename... Tail>
struct tuple_element<N, Tuple<Head, Tail...>>
    : tuple_element<N - 1, Tuple<Tail...>> {};

template <size_t N, typename T>
using tuple_element_t = typename tuple_element<N, T>::type;

// get<N>
template <size_t N, typename... Ts>
tuple_element_t<N, Tuple<Ts...>>& get(Tuple<Ts...>& t) {
  using Leaf = detail::TupleLeaf<N, tuple_element_t<N, Tuple<Ts...>>>;
  return static_cast<Leaf&>(t).value;
}

template <size_t N, typename... Ts>
const tuple_element_t<N, Tuple<Ts...>>& get(const Tuple<Ts...>& t) {
  using Leaf = detail::TupleLeaf<N, tuple_element_t<N, Tuple<Ts...>>>;
  return static_cast<const Leaf&>(t).value;
}

template <size_t N, typename... Ts>
tuple_element_t<N, Tuple<Ts...>>&& get(Tuple<Ts...>&& t) {
  using Leaf = detail::TupleLeaf<N, tuple_element_t<N, Tuple<Ts...>>>;
  return fk::types::move(static_cast<Leaf&>(t).value);
}

// make_tuple
template <typename... Ts>
Tuple<Ts...> make_tuple(Ts... args) {
  return Tuple<Ts...>(fk::types::move(args)...);
}

} // namespace utilities
} // namespace fk
