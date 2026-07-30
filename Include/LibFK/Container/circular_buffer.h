#pragma once

#include <LibC/stddef.h>

namespace fk {
namespace containers {

template <typename T, size_t N>
class CircularBuffer {
public:
    CircularBuffer() = default;

    bool is_empty() const { return m_size == 0; }
    bool is_full() const { return m_size == N; }
    size_t size() const { return m_size; }
    size_t capacity() const { return N; }

    void enqueue(const T& value) {
        m_data[m_head] = value;
        m_head = (m_head + 1) % N;
        
        if (is_full()) {
            m_tail = (m_tail + 1) % N;
        } else {
            m_size++;
        }
    }

    T dequeue() {
        T value = m_data[m_tail];
        m_tail = (m_tail + 1) % N;
        m_size--;
        return value;
    }

    void remove_last() {
        if (is_empty()) return;
        m_head = (m_head == 0) ? (N - 1) : (m_head - 1);
        m_size--;
    }

    T& operator[](size_t index) { return m_data[(m_tail + index) % N]; }

    const T& operator[](size_t index) const {
        return m_data[(m_tail + index) % N];
    }

    size_t space_available() const { return N - m_size; }

    // Non-overwriting bulk push: stops at capacity.
    void push_range(const T* data, size_t count) {
        if (count > space_available()) count = space_available();
        for (size_t i = 0; i < count; ++i) {
            m_data[m_head] = data[i];
            m_head = (m_head + 1) % N;
        }
        m_size += count;
    }

    // Read and consume count elements into out[].
    void pop_range(T* out, size_t count) {
        if (count > m_size) count = m_size;
        for (size_t i = 0; i < count; ++i) {
            out[i] = m_data[m_tail];
            m_tail = (m_tail + 1) % N;
        }
        m_size -= count;
    }

    // Discard count elements from the front without copying.
    void discard(size_t count) {
        if (count > m_size) count = m_size;
        m_tail = (m_tail + count) % N;
        m_size -= count;
    }

    void clear() {
        if (!__is_trivially_destructible(T)) {
            for (size_t i = 0; i < m_size; ++i) {
                size_t idx = (m_tail + i) % N;
                m_data[idx].~T();
                new (&m_data[idx]) T{};
            }
        }
        m_head = 0;
        m_tail = 0;
        m_size = 0;
    }

private:
    T m_data[N];
    size_t m_head = 0;
    size_t m_tail = 0;
    size_t m_size = 0;
};

} // namespace containers
} // namespace fk
