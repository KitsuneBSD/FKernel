#pragma once

#include <LibFK/Container/Sequence/vector.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

template <typename T>
class PriorityQueue {
public:
  PriorityQueue() = default;

  void push(const T &value) {
    m_heap.push_back(value);
    sift_up(m_heap.size() - 1);
  }

  void push(T &&value) {
    m_heap.push_back(fk::types::move(value));
    sift_up(m_heap.size() - 1);
  }

  const T &top() const { return m_heap[0]; }

  void pop() {
    if (is_empty())
      return;
    m_heap[0] = fk::types::move(m_heap[m_heap.size() - 1]);
    m_heap.pop_back();
    if (!is_empty())
      sift_down(0);
  }

  size_t size() const { return m_heap.size(); }
  bool is_empty() const { return m_heap.is_empty(); }
  void clear() { m_heap.clear(); }

private:
  void sift_up(size_t idx) {
    while (idx > 0) {
      size_t parent = (idx - 1) / 2;
      if (m_heap[parent] < m_heap[idx])
        swap_at(parent, idx);
      else
        break;
      idx = parent;
    }
  }

  void sift_down(size_t idx) {
    size_t n = m_heap.size();
    while (true) {
      size_t largest = idx;
      size_t left = 2 * idx + 1;
      size_t right = 2 * idx + 2;
      if (left < n && m_heap[largest] < m_heap[left])
        largest = left;
      if (right < n && m_heap[largest] < m_heap[right])
        largest = right;
      if (largest == idx)
        break;
      swap_at(idx, largest);
      idx = largest;
    }
  }

  void swap_at(size_t a, size_t b) {
    T tmp = fk::types::move(m_heap[a]);
    m_heap[a] = fk::types::move(m_heap[b]);
    m_heap[b] = fk::types::move(tmp);
  }

  Vector<T> m_heap;
};

} // namespace containers
} // namespace fk
