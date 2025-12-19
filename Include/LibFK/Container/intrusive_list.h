#pragma once

#include <LibC/stddef.h>

namespace fk {
namespace containers {

template <typename T>
struct IntrusiveListNode {
    T* prev = nullptr;
    T* next = nullptr;
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
template <typename T, IntrusiveListNode<T> T::* NodeMember>
class IntrusiveList {
public:
    IntrusiveList() = default;

    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    bool empty() const { return m_size == 0; }
    size_t size() const { return m_size; }

    T* front() { return m_head; }
    T* back()  { return m_tail; }

    void push_back(T* obj) {
        if (!obj) return;

        auto& node = obj->*NodeMember;
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

    void push_front(T* obj) {
        if (!obj) return;

        auto& node = obj->*NodeMember;
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

    void remove(T* obj) {
        if (!obj) return;

        auto& node = obj->*NodeMember;

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

    T* pop_front() {
        if (!m_head) return nullptr;
        T* obj = m_head;
        remove(obj);
        return obj;
    }

    /* iterator mínimo (scheduler-safe) */
    class iterator {
        T* cur;
    public:
        explicit iterator(T* t) : cur(t) {}

        T& operator*() const { return *cur; }
        T* operator->() const { return cur; }

        iterator& operator++() {
            cur = cur ? (cur->*NodeMember).next : nullptr;
            return *this;
        }

        bool operator!=(const iterator& other) const {
            return cur != other.cur;
        }
    };

    iterator begin() { return iterator(m_head); }
    iterator end()   { return iterator(nullptr); }

private:
    T* m_head = nullptr;
    T* m_tail = nullptr;
    size_t m_size = 0;
};

} // namespace containers
} // namespace fk
