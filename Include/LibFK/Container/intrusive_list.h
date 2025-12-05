#pragma once

#include <LibC/stddef.h> // For size_t
#include <LibFK/Types/types.h> // For fk::types::move

namespace fk {
namespace containers {

template <typename T>
struct IntrusiveListNode {
    T* prev = nullptr;
    T* next = nullptr;
};

/**
 * @brief An intrusive doubly-linked list.
 *
 * In an intrusive list, the list nodes are part of the objects themselves,
 * rather than separate allocations. This means:
 * - T must contain an `IntrusiveListNode<T> m_list_node;` member.
 * - No dynamic memory allocation is performed by the list itself.
 * - Objects added to the list must be heap-allocated (or have static/global storage)
 *   and their lifetime is managed externally.
 * - The list only manages pointers, not object ownership.
 *
 * This design is common in kernel development to avoid overheads of dynamic
 * node allocation and to allow objects to be part of multiple lists
 * (with multiple `IntrusiveListNode` members).
 *
 * @tparam T The type of object to be stored in the list.
 *           T must have a public member `IntrusiveListNode<T> m_list_node;`.
 */
template <typename T>
class IntrusiveList {
public:
    IntrusiveList() = default;

    // Not copyable or assignable, as it manages raw pointers to nodes embedded in objects.
    IntrusiveList(const IntrusiveList&) = delete;
    IntrusiveList& operator=(const IntrusiveList&) = delete;

    // Move constructor and assignment are fine, as they just transfer pointers.
    IntrusiveList(IntrusiveList&& other) noexcept
        : m_metadata(other.m_metadata) {
        other.m_metadata.m_head = nullptr;
        other.m_metadata.m_tail = nullptr;
        other.m_metadata.m_size = 0;
    }

    IntrusiveList& operator=(IntrusiveList&& other) noexcept {
        if (this != &other) {
            clear(); // Clear existing nodes, if any (does not delete objects)
            m_metadata = other.m_metadata;
            other.m_metadata.m_head = nullptr;
            other.m_metadata.m_tail = nullptr;
            other.m_metadata.m_size = 0;
        }
        return *this;
    }

    /**
     * @brief Adds an object to the back of the list.
     * @param obj A pointer to the object to add. The object must contain an
     *            `IntrusiveListNode<T> m_list_node;` member.
     */
    void push_back(T* obj) {
        if (!obj) {
            return; // Or assert, depending on desired error handling
        }

        IntrusiveListNode<T>& node = obj->m_list_node;
        node.next = nullptr;
        node.prev = m_metadata.m_tail;

        if (empty()) {
            m_metadata.m_head = obj;
            m_metadata.m_tail = obj;
        } else {
            m_metadata.m_tail->m_list_node.next = obj;
            m_metadata.m_tail = obj;
        }
        m_metadata.m_size++;
    }

    /**
     * @brief Adds an object to the front of the list.
     * @param obj A pointer to the object to add.
     */
    void push_front(T* obj) {
        if (!obj) {
            return;
        }

        IntrusiveListNode<T>& node = obj->m_list_node;
        node.prev = nullptr;
        node.next = m_metadata.m_head;

        if (empty()) {
            m_metadata.m_head = obj;
            m_metadata.m_tail = obj;
        } else {
            m_metadata.m_head->m_list_node.prev = obj;
            m_metadata.m_head = obj;
        }
        m_metadata.m_size++;
    }

    /**
     * @brief Removes an object from the list.
     *
     * @param obj A pointer to the object to remove. The object must currently
     *            be in this list.
     */
  void _remove_node_from_prev(IntrusiveListNode<T>& node) {
    if (node.prev) {
      node.prev->m_list_node.next = node.next;
    } else {
      m_metadata.m_head = node.next;
    }
  }

  void _remove_node_from_next(IntrusiveListNode<T>& node) {
    if (node.next) {
      node.next->m_list_node.prev = node.prev;
    } else {
      m_metadata.m_tail = node.prev;
    }
  }

    void remove(T* obj) {
        if (!obj || !contains(obj)) { // Ensure obj is not null and is in this list
            return;
        }

        IntrusiveListNode<T>& node = obj->m_list_node;

        _remove_node_from_prev(node);
        _remove_node_from_next(node);

        node.prev = nullptr; // Clear pointers in the removed node
        node.next = nullptr;
        m_metadata.m_size--;
    }

    /**
     * @brief Removes and returns the object at the front of the list.
     * @return A pointer to the removed object, or nullptr if the list is empty.
     */
    T* pop_front() {
        if (empty()) {
            return nullptr;
        }
        T* obj = m_metadata.m_head;
        remove(obj); // Reuse remove logic
        return obj;
    }

    /**
     * @brief Removes and returns the object at the back of the list.
     * @return A pointer to the removed object, or nullptr if the list is empty.
     */
    T* pop_back() {
        if (empty()) {
            return nullptr;
        }
        T* obj = m_metadata.m_tail;
        remove(obj); // Reuse remove logic
        return obj;
    }

    /**
     * @brief Checks if the list contains a specific object.
     * This is an O(N) operation. For frequent lookups, consider other data structures.
     * @param obj A pointer to the object to check.
     * @return True if the object is in the list, false otherwise.
     */
    bool contains(T* obj) const {
        if (!obj) {
            return false;
        }
        for (T* current = m_metadata.m_head; current != nullptr; current = current->m_list_node.next) {
            if (current == obj) {
                return true;
            }
        }
        return false;
    }

    /**
     * @brief Removes all elements from the list without deleting the objects.
     * The objects themselves are not deallocated; only their `m_list_node`
     * pointers are reset and the list's internal pointers are cleared.
     */
    void clear() {
        T* current = m_metadata.m_head;
        while (current) {
            T* next_obj = current->m_list_node.next;
            current->m_list_node.prev = nullptr;
            current->m_list_node.next = nullptr;
            current = next_obj;
        }
        m_metadata.m_head = nullptr;
        m_metadata.m_tail = nullptr;
        m_metadata.m_size = 0;
    }

    /**
     * @brief Returns the object at the front of the list without removing it.
     * @return A pointer to the front object, or nullptr if the list is empty.
     */
    T* front() {
        return m_metadata.m_head;
    }

    const T* front() const {
        return m_metadata.m_head;
    }

    /**
     * @brief Returns the object at the back of the list without removing it.
     * @return A pointer to the back object, or nullptr if the list is empty.
     */
    T* back() {
        return m_metadata.m_tail;
    }

    const T* back() const {
        return m_metadata.m_tail;
    }

    /**
     * @brief Gets the number of elements in the list.
     * @return The current size of the list.
     */
    size_t size() const {
        return m_metadata.m_size;
    }

    /**
     * @brief Checks if the list is empty.
     * @return True if the list contains no elements, false otherwise.
     */
    bool empty() const {
        return m_metadata.m_size == 0;
    }

    // --- Iterator support ---
    class iterator {
    private:
        T* m_current;
    public:
        iterator(T* node_ptr) : m_current(node_ptr) {}

        T& operator*() const { return *m_current; }
        T* operator->() const { return m_current; }

        iterator& operator++() { // Pre-increment
            if (m_current) m_current = m_current->m_list_node.next;
            return *this;
        }

        iterator operator++(int) { // Post-increment
            iterator temp = *this;
            if (m_current) m_current = m_current->m_list_node.next;
            return temp;
        }

        iterator& operator--() { // Pre-decrement
            if (m_current) m_current = m_current->m_list_node.prev;
            return *this;
        }

        iterator operator--(int) { // Post-decrement
            iterator temp = *this;
            if (m_current) m_current = m_current->m_list_node.prev;
            return temp;
        }

        bool operator==(const iterator& other) const { return m_current == other.m_current; }
        bool operator!=(const iterator& other) const { return m_current != other.m_current; }
    };

    class const_iterator {
    private:
        const T* m_current;
    public:
        const_iterator(const T* node_ptr) : m_current(node_ptr) {}

        const T& operator*() const { return *m_current; }
        const T* operator->() const { return m_current; }

        const_iterator& operator++() { // Pre-increment
            if (m_current) m_current = m_current->m_list_node.next;
            return *this;
        }

        const_iterator operator++(int) { // Post-increment
            const_iterator temp = *this;
            if (m_current) m_current = m_current->m_list_node.next;
            return temp;
        }

        const_iterator& operator--() { // Pre-decrement
            if (m_current) m_current = m_current->m_list_node.prev;
            return *this;
        }

        const_iterator operator--(int) { // Post-decrement
            const_iterator temp = *this;
            if (m_current) m_current = m_current->m_list_node.prev;
            return temp;
        }

        bool operator==(const const_iterator& other) const { return m_current == other.m_current; }
        bool operator!=(const const_iterator& other) const { return m_current != other.m_current; }
    };

    iterator begin() { return iterator(m_metadata.m_head); }
    iterator end() { return iterator(nullptr); } // Sentinel for end

    const_iterator begin() const { return const_iterator(m_metadata.m_head); }
    const_iterator end() const { return const_iterator(nullptr); } // Sentinel for end

    const_iterator cbegin() const { return const_iterator(m_metadata.m_head); }
    const_iterator cend() const { return const_iterator(nullptr); } // Sentinel for end

private:
  struct ListMetadata {
    T* m_head = nullptr;
    T* m_tail = nullptr;
    size_t m_size = 0;
  };
  ListMetadata m_metadata;
};

} // namespace containers
} // namespace fk
