#pragma once

#include <LibC/stddef.h>

namespace fk {
namespace containers {

template <typename T> struct IntrusiveListNode {
  T *prev = nullptr;
  T *next = nullptr;
};

/**
 * Intrusive doubly-linked list using pointer-to-member.
 *
 * Permite que um mesmo objeto participe de múltiplas listas
 * usando diferentes IntrusiveListNode.
 *
 * Exemplo:
 *   IntrusiveList<Task, &Task::run_node>
 *   IntrusiveList<Task, &Task::sleep_node>
 */
template <typename T, IntrusiveListNode<T> T::*NodeMember> class IntrusiveList {
public:
  IntrusiveList() = default;

  IntrusiveList(const IntrusiveList &) = delete;
  IntrusiveList &operator=(const IntrusiveList &) = delete;

  bool empty() const { return m_size == 0; }
  size_t size() const { return m_size; }

  T *front() { return m_head; }
  T *back() { return m_tail; }

  bool is_empty() const { return empty(); }

  T *first() { return front(); }

  void append(T &obj) { push_back(&obj); }

  void prepend(T &obj) { push_front(&obj); }

  void remove(T &obj) { remove(&obj); }
  void push_back(T *obj) {
    if (!obj)
      return;

    auto &node = obj->*NodeMember;
    if (node.prev != nullptr || node.next != nullptr)
      return;

    node.prev = m_tail;
    node.next = nullptr;

    if (!m_head) {
      m_head = obj;
    } else {
      (m_tail->*NodeMember).next = obj;
    }

    m_tail = obj;
    ++m_size;
  }

  void push_front(T *obj) {
    if (!obj)
      return;

    auto &node = obj->*NodeMember;
    if (node.prev != nullptr || node.next != nullptr)
      return;

    node.prev = nullptr;
    node.next = m_head;

    if (!m_head) {
      m_tail = obj;
    } else {
      (m_head->*NodeMember).prev = obj;
    }

    m_head = obj;
    ++m_size;
  }

  void remove(T *obj) {
    if (!obj)
      return;

    auto &node = obj->*NodeMember;

    // Guard: if the node is not linked and not the head, it's not in this list.
    // Prevents size underflow and head/tail corruption on double-remove.
    if (node.prev == nullptr && node.next == nullptr && m_head != obj)
      return;

    if (node.prev) {
      (node.prev->*NodeMember).next = node.next;
    } else {
      m_head = node.next;
    }

    if (node.next) {
      (node.next->*NodeMember).prev = node.prev;
    } else {
      m_tail = node.prev;
    }

    node.prev = nullptr;
    node.next = nullptr;
    --m_size;
  }

  // Insert obj immediately before position.
  // If position is nullptr, appends to the back.
  void insert_before(T *position, T *obj) {
    if (!obj) return;
    if (!position) { push_back(obj); return; }

    auto &node     = obj->*NodeMember;
    auto &pos_node = position->*NodeMember;

    node.prev = pos_node.prev;
    node.next = position;

    if (pos_node.prev)
      (pos_node.prev->*NodeMember).next = obj;
    else
      m_head = obj;

    pos_node.prev = obj;
    ++m_size;
  }

  // Insert obj in sorted order using comparator cmp(obj, candidate) → true means obj comes before candidate.
  template <typename Cmp>
  void insert_sorted(T *obj, Cmp &&cmp) {
    if (!obj) return;
    T *pos = m_head;
    while (pos) {
      if (cmp(obj, pos)) break;
      pos = (pos->*NodeMember).next;
    }
    insert_before(pos, obj);
  }

  T *pop_front() {
    if (!m_head)
      return nullptr;
    T *obj = m_head;
    remove(obj);
    return obj;
  }

  /* iterator mínimo (scheduler-safe) */
  class iterator {
    T *cur;

  public:
    explicit iterator(T *t) : cur(t) {}

    T &operator*() const { return *cur; }
    T *operator->() const { return cur; }

    iterator &operator++() {
      cur = cur ? (cur->*NodeMember).next : nullptr;
      return *this;
    }

    bool operator!=(const iterator &other) const { return cur != other.cur; }
  };

  iterator begin() { return iterator(m_head); }
  iterator end() { return iterator(nullptr); }

private:
  T *m_head = nullptr;
  T *m_tail = nullptr;
  size_t m_size = 0;
};

} // namespace containers
} // namespace fk
