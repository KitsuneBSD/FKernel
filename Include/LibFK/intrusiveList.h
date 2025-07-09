#pragma once

namespace FK {

template<typename Type>
struct IntrusiveNode {
    Type* next = nullptr;
    Type* prev = nullptr;

    void detach();
    bool is_linked();
};

template<typename T, IntrusiveNode<T> T::* member>
class IntrusiveList {
private:
    T* head_ = nullptr;
    T* tail_ = nullptr;

public:
    constexpr IntrusiveList() = default;

    bool is_empty() const noexcept;
    T* head() noexcept;
    T* tail() noexcept;

    void append(T* node) noexcept;
    void prepend(T* node) noexcept;
    void remove(T* node) noexcept;
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
