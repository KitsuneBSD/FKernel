#pragma once

#include "LibFK/log.h"
namespace FK {
template<typename Type>
struct IntrusiveNode {
    Type* next = nullptr;
    Type* prev = nullptr;

    void detach()
    {
        Logf(LogLevel::TRACE, "IntrusiveNode: detach this=%p next=%p prev=%p", this, next, prev);

        if (prev)
            prev->next = next;
        if (next)
            next->prev = prev;
        next = nullptr;
        prev = nullptr;
    }

    bool is_linked() const { return next || prev; }
};

template<typename T, IntrusiveNode<T> T::* member>
class IntrusiveList {
private:
    T* head_ = nullptr;
    T* tail_ = nullptr;

public:
    constexpr IntrusiveList() = default;

    bool is_empty() const noexcept { return head_ == nullptr; }
    T* head() noexcept { return head_; }
    T* tail() noexcept { return tail_; }

    void append(T* node) noexcept
    {
        auto& n = node->*member;

        if (n.is_linked()) {
            Logf(LogLevel::WARN, "IntrusiveList: append: node already linked, skipping: %p", node);
            return;
        }

        n.prev = tail_;
        n.next = nullptr;

        if (tail_)
            (tail_->*member).next = node;
        else
            head_ = node;

        tail_ = node;
    }

    void remove(T* node) noexcept
    {
        auto& n = node->*member;

        Logf(LogLevel::TRACE, "IntrusiveList: remove node=%p", node);

        if (n.prev)
            (n.prev->*member).next = n.next;
        else
            head_ = n.next;

        if (n.next)
            (n.next->*member).prev = n.prev;
        else
            tail_ = n.prev;

        n.next = nullptr;
        n.prev = nullptr;
    }

    template<typename Callback>
    void for_each(Callback callback_function) noexcept
    {
        for (T* it = head_; it; it = (it->*member).next)
            callback_function(it);
    }

    class Iterator {
        T* m_node;

    public:
        explicit Iterator(T* node)
            : m_node(node)
        {
        }
        T& operator*() const { return *m_node; }
        T* operator->() const { return m_node; }
        Iterator& operator++()
        {
            m_node = (m_node->*member).next;
            return *this;
        }
        bool operator!=(Iterator const& other) const { return m_node != other.m_node; }
    };

    Iterator begin() const { return Iterator(head_); }
    Iterator end() const { return Iterator(nullptr); }
};
}
