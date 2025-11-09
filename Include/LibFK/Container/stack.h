#pragma once

#include <LibFK/Container/array.h>
#include <LibFK/Types/types.h>

template <typename Type, size_t MAX_SIZE = 4096> class Stack {
private:
  array<Type, MAX_SIZE> m_stack;
  size_t top_index = 0;

  bool is_full() { return top_index >= MAX_SIZE; }
  bool is_empty() { return top_index == 0; }

public:
  constexpr Stack() : top_index(0) {}

  size_t size() const { return top_index; }
  size_t capacity() const { return MAX_SIZE; }

  bool push(const Type &value) {
    if (is_full())
      return false;

    m_stack[++top_index] = value;
    return true;
  }

  bool pop(Type &out) {
    if (is_empty())
      return false;

    out = m_stack[--top_index];
    return true;
  }

  bool peek(Type &out) {
    if (is_empty())
      return false;

    out = m_stack[top_index - 1];
    return true;
  }

  void clear() { top_index = 0; }
};
