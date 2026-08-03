#pragma once

#include <LibFK/Memory/Allocators/heap_malloc.h>
#include <LibFK/Types/types.h>

namespace fk {
namespace containers {

template <typename T>
class ForwardList {
    struct Node {
        T value;
        Node* next{nullptr};
        explicit Node(const T& v) : value(v) {}
        explicit Node(T&& v) : value(fk::types::move(v)) {}
    };

    Node* m_head{nullptr};
    size_t m_size{0};

public:
    ForwardList() = default;

    ~ForwardList() { clear(); }

    ForwardList(const ForwardList&) = delete;
    ForwardList& operator=(const ForwardList&) = delete;

    void push_front(const T& value) {
        auto* n = new Node(value);
        n->next = m_head;
        m_head = n;
        m_size++;
    }

    void push_front(T&& value) {
        auto* n = new Node(fk::types::move(value));
        n->next = m_head;
        m_head = n;
        m_size++;
    }

    void pop_front() {
        if (!m_head) return;
        auto* old = m_head;
        m_head = m_head->next;
        delete old;
        m_size--;
    }

    T& front() { return m_head->value; }
    const T& front() const { return m_head->value; }

    bool is_empty() const { return m_head == nullptr; }
    size_t size() const { return m_size; }

    void clear() {
        while (m_head) pop_front();
    }

    struct Iterator {
        Node* m_node;
        explicit Iterator(Node* n) : m_node(n) {}
        T& operator*() { return m_node->value; }
        T* operator->() { return &m_node->value; }
        Iterator& operator++() { m_node = m_node->next; return *this; }
        bool operator==(const Iterator& o) const { return m_node == o.m_node; }
        bool operator!=(const Iterator& o) const { return m_node != o.m_node; }
    };

    Iterator begin() { return Iterator(m_head); }
    Iterator end()   { return Iterator(nullptr); }

    struct ConstIterator {
        const Node* m_node;
        explicit ConstIterator(const Node* n) : m_node(n) {}
        const T& operator*() const { return m_node->value; }
        const T* operator->() const { return &m_node->value; }
        ConstIterator& operator++() { m_node = m_node->next; return *this; }
        bool operator==(const ConstIterator& o) const { return m_node == o.m_node; }
        bool operator!=(const ConstIterator& o) const { return m_node != o.m_node; }
    };

    ConstIterator begin() const { return ConstIterator(m_head); }
    ConstIterator end()   const { return ConstIterator(nullptr); }

    Iterator insert_after(Iterator pos, const T& value) {
        auto* n = new Node(value);
        n->next = pos.m_node->next;
        pos.m_node->next = n;
        m_size++;
        return Iterator(n);
    }

    Iterator erase_after(Iterator pos) {
        if (!pos.m_node || !pos.m_node->next) return end();
        auto* to_del = pos.m_node->next;
        pos.m_node->next = to_del->next;
        delete to_del;
        m_size--;
        return Iterator(pos.m_node->next);
    }
};

} // namespace containers
} // namespace fk
