#pragma once

#include <LibFK/enforce.h>
#include <LibFK/log.h>

namespace FK {
template<typename Type>
struct IntrusiveNode {
    Type* next = nullptr;
    Type* prev = nullptr;
    bool linked = false;

    bool is_linked() const noexcept
    {
        return linked;
    }

    void link() { linked = true; }
    void unlink() { linked = false; }
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
        FK::enforce(node != nullptr, "IntrusiveList: append called with nullptr");
        auto& n = node->*member;
        FK::enforcef(!n.is_linked(), "IntrusiveList: append called on already linked node %p", node);

        n.prev = tail_;
        n.next = nullptr;

        if (tail_)
            (tail_->*member).next = node;
        else
            head_ = node;

        tail_ = node;
        n.link();
    }

    void remove(T* node) noexcept
    {
        FK::enforce(node != nullptr, "IntrusiveList: remove called with nullptr");
        auto& n = node->*member;
        FK::enforcef(n.is_linked(), "IntrusiveList: remove called on unlinked node %p", node);

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
        n.unlink();
    }

    void set_head(T* node) noexcept
    {
        FK::enforcef(node == nullptr || (node->*member).is_linked(),
            "IntrusiveList: set_head called with invalid node %p", node);
        head_ = node;
    }

    template<typename Callback>
    void for_each(Callback callback_function) noexcept
    {
        for (T* it = head_; it; it = (it->*member).next)
            callback_function(it);
    }

    template<typename Comparator>
    void insert_ordered(T* node, Comparator comp) noexcept
    {
        FK::enforce(node != nullptr, "IntrusiveList: insert_ordered called with nullptr");
        auto& n = node->*member;
        FK::enforcef(!n.is_linked(), "IntrusiveList: insert_ordered called on already linked node %p", node);

        if (is_empty()) {
            append(node);
            return;
        }

        for (T* it = head_; it; it = (it->*member).next) {
            if (comp(node, it)) {
                auto& it_node = it->*member;

                n.prev = it_node.prev;
                n.next = it;

                if (it_node.prev)
                    (it_node.prev->*member).next = node;
                else
                    head_ = node;

                it_node.prev = node;
                n.link();
                return;
            }
        }
        append(node);
    }

    class Iterator {
        T* m_node;

    public:
        explicit Iterator(T* node) noexcept
            : m_node(node)
        {
        }
        T& operator*() const noexcept { return *m_node; }
        T* operator->() const noexcept { return m_node; }
        Iterator& operator++() noexcept
        {
            m_node = (m_node->*member).next;
            return *this;
        }
        bool operator!=(Iterator const& other) const noexcept { return m_node != other.m_node; }
        bool operator==(Iterator const& other) const noexcept { return m_node == other.m_node; }
    };

    Iterator begin() const noexcept { return Iterator(head_); }
    Iterator end() const noexcept { return Iterator(nullptr); }
};
}
