#include <LibFK/intrusiveList.h>

namespace FK {
template<typename Type>
void IntrusiveNode<Type>::detach()
{
    next = nullptr;
    prev = nullptr;
}

template<typename Type>
bool IntrusiveNode<Type>::is_linked()
{
    return prev || next;
}
template<typename T, IntrusiveNode<T> T::* member>
bool IntrusiveList<T, member>::is_empty() const noexcept { return head_ == nullptr; }

template<typename T, IntrusiveNode<T> T::* member>
T* IntrusiveList<T, member>::head() noexcept { return head_; }

template<typename T, IntrusiveNode<T> T::* member>
T* IntrusiveList<T, member>::tail() noexcept { return tail_; }

template<typename T, IntrusiveNode<T> T::* member>
void IntrusiveList<T, member>::append(T* node) noexcept
{
    auto& n = node->*member;
    n.next = nullptr;
    n.prev = tail_;

    if (tail_)
        (tail_->*member).next = node;
    else
        head_ = node;

    tail_ = node;
}
template<typename T, IntrusiveNode<T> T::* member>
void IntrusiveList<T, member>::prepend(T* node) noexcept
{
    auto& n = node->*member;
    n.prev = nullptr;
    n.next = head_;

    if (head_)
        (head_->*member).prev = node;
    else
        tail_ = node;

    head_ = node;
}
template<typename T, IntrusiveNode<T> T::* member>
void IntrusiveList<T, member>::remove(T* node) noexcept
{
    auto& n = node->*member;

    if (n.prev)
        (n.prev->*member).next = n.next;
    else
        head_ = n.next;

    if (n.next)
        (n.next->*member).prev = n.prev;
    else
        tail_ = n.prev;

    n.detach();
}
}
