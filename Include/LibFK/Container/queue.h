#pragma once

#include <LibC/stddef.h> // For size_t
#include <LibFK/Container/intrusive_list.h> // Include IntrusiveList

namespace fk {
namespace containers {

/**
 * @brief A basic FIFO (First-In, First-Out) queue using an IntrusiveList.
 *
 * This queue is intrusive, meaning that the elements stored in it must
 * contain an `IntrusiveListNode<T> m_list_node;` member.
 *
 * @tparam T The type of object to be stored in the queue.
 *           T must have a public member `IntrusiveListNode<T> m_list_node;`.
 */
template <typename T>
class Queue {
public:
    Queue() = default;

    // Not copyable or assignable, as it manages raw pointers to nodes embedded in objects.
    Queue(const Queue&) = delete;
    Queue& operator=(const Queue&) = delete;

    // Move constructor and assignment are fine, as they just transfer pointers.
    Queue(Queue&& other) noexcept = default;
    Queue& operator=(Queue&& other) noexcept = default;

    /**
     * @brief Adds an object to the back of the queue (enqueues it).
     * @param obj A pointer to the object to add. The object must contain an
     *            `IntrusiveListNode<T> m_list_node;` member.
     */
    void enqueue(T* obj) {
        m_list.push_back(obj);
    }

    /**
     * @brief Removes and returns the object at the front of the queue (dequeues it).
     * @return A pointer to the removed object, or nullptr if the queue is empty.
     */
    T* dequeue() {
        return m_list.pop_front();
    }

    /**
     * @brief Returns the object at the front of the queue without removing it.
     * @return A pointer to the front object, or nullptr if the queue is empty.
     */
    T* front() {
        return m_list.front();
    }

    /**
     * @brief Returns the object at the front of the queue without removing it (const version).
     * @return A const pointer to the front object, or nullptr if the queue is empty.
     */
    const T* front() const {
        return m_list.front();
    }

    /**
     * @brief Returns the object at the back of the queue without removing it.
     * @return A pointer to the back object, or nullptr if the queue is empty.
     */
    T* back() {
        return m_list.back();
    }

    /**
     * @brief Returns the object at the back of the queue without removing it (const version).
     * @return A const pointer to the back object, or nullptr if the queue is empty.
     */
    const T* back() const {
        return m_list.back();
    }

    /**
     * @brief Checks if the queue is empty.
     * @return True if the queue contains no elements, false otherwise.
     */
    bool empty() const {
        return m_list.empty();
    }

    /**
     * @brief Gets the number of elements in the queue.
     * @return The current size of the queue.
     */
    size_t size() const {
        return m_list.size();
    }

    /**
     * @brief Removes all elements from the queue without deleting the objects.
     * The objects themselves are not deallocated; only their `m_list_node`
     * pointers are reset and the queue's internal pointers are cleared.
     */
    void clear() {
        m_list.clear();
    }

private:
    IntrusiveList<T> m_list;
};

} // namespace containers
} // namespace fk
