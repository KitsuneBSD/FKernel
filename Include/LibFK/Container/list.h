
#pragma once

#include <LibFK/Core/Result.h>
#include <LibFK/Memory/own_ptr.h>
#include <LibFK/Types/types.h> // For Intrinsic and move

namespace fk {
namespace containers {

template <typename T>
class List {
private:
    struct Node {
        fk::OwnPtr<T> data;
        Node* next = nullptr;
    };

    Node* m_head = nullptr;
    Node* m_tail = nullptr;
    size_t m_size = 0;

public:
    // Constructor
    List() = default;

    // Destructor
    ~List() {
        clear();
    }

    // Copy constructor (deleted for simplicity, can be implemented if needed)
    List(const List&) = delete;
    // Move constructor
    List(List&& other) noexcept : m_head(other.m_head), m_tail(other.m_tail), m_size(other.m_size) {
        other.m_head = nullptr;
        other.m_tail = nullptr;
        other.m_size = 0;
    }

    // Copy assignment operator (deleted)
    List& operator=(const List&) = delete;
    // Move assignment operator
    List& operator=(List&& other) noexcept {
        if (this != &other) {
            clear();
            m_head = other.m_head;
            m_tail = other.m_tail;
            m_size = other.m_size;
            other.m_head = nullptr;
            other.m_tail = nullptr;
            other.m_size = 0;
        }
        return *this;
    }

    // Add element to the end of the list
    fk::core::Result<void, fk::core::Error> push_back(T value) {
        // Use make_own to create an OwnPtr for the data
        auto owned_data = fk::memory::make_own<T>(fk::move(value));
        if (owned_data.is_error()) {
            // Allocation failed
            return fk::core::Error::OutOfMemory;
        }

        Node* new_node = new Node(); // Allocate a new node
        if (!new_node) {
            // Allocation failed for the node itself
            return fk::core::Error::OutOfMemory;
        }
        new_node->data = fk::move(owned_data.value()); // Transfer ownership to the node

        if (!m_head) { // If list is empty
            m_head = new_node;
            m_tail = new_node;
        } else {
            m_tail->next = new_node;
            m_tail = new_node;
        }
        m_size++;
        return fk::core::Result<void>(); // Success
    }

    // Remove all elements from the list
    void clear() {
        Node* current = m_head;
        while (current) {
            Node* next = current->next;
            delete current; // This will call the destructor of Node, which in turn deletes OwnPtr<T>
            current = next;
        }
        m_head = nullptr;
        m_tail = nullptr;
        m_size = 0;
    }

    // Get the number of elements
    size_t size() const {
        return m_size;
    }

    // Check if the list is empty
    bool empty() const {
        return m_size == 0;
    }

    // Get the first element (optional, for convenience)
    fk::core::Result<const T*, fk::core::Error> front() const {
        if (!m_head) {
            return fk::core::Error::NotFound; // Or a specific "EmptyList" error
        }
        return fk::core::Result<const T*>(&m_head->data.value());
    }

    fk::core::Result<T*, fk::core::Error> front() {
        if (!m_head) {
            return fk::core::Error::NotFound; // Or a specific "EmptyList" error
        }
        return fk::core::Result<T*>(&m_head->data.value());
    }

    // Get the last element (optional, for convenience)
    fk::core::Result<const T*, fk::core::Error> back() const {
        if (!m_tail) {
            return fk::core::Error::NotFound; // Or a specific "EmptyList" error
        }
        return fk::core::Result<const T*>(&m_tail->data.value());
    }

    fk::core::Result<T*, fk::core::Error> back() {
        if (!m_tail) {
            return fk::core::Error::NotFound; // Or a specific "EmptyList" error
        }
        return fk::core::Result<T*>(&m_tail->data.value());
    }

    // Basic iterator support (optional, but good practice)
    class iterator {
    private:
        Node* m_current;
    public:
        iterator(Node* node) : m_current(node) {}

        T& operator*() const { return m_current->data.value(); }
        T* operator->() const { return &m_current->data.value(); }

        iterator& operator++() { // Pre-increment
            if (m_current) m_current = m_current->next;
            return *this;
        }

        bool operator==(const iterator& other) const { return m_current == other.m_current; }
        bool operator!=(const iterator& other) const { return m_current != other.m_current; }
    };

    iterator begin() { return iterator(m_head); }
    iterator end() { return iterator(nullptr); } // Sentinel for end

    // Const iterator support
    class const_iterator {
    private:
        const Node* m_current;
    public:
        const_iterator(const Node* node) : m_current(node) {}

        const T& operator*() const { return m_current->data.value(); }
        const T* operator->() const { return &m_current->data.value(); }

        const_iterator& operator++() { // Pre-increment
            if (m_current) m_current = m_current->next;
            return *this;
        }

        bool operator==(const const_iterator& other) const { return m_current == other.m_current; }
        bool operator!=(const const_iterator& other) const { return m_current != other.m_current; }
    };

    const_iterator begin() const { return const_iterator(m_head); }
    const_iterator end() const { return const_iterator(nullptr); } // Sentinel for end
};

} // namespace containers
} // namespace fk
