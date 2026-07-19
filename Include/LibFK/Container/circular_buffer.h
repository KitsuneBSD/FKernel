#pragma once

#include <LibC/stddef.h>
#include <LibFK/Core/Assertions.h>

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
        assert(!is_empty());
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

    T& operator[](size_t index) {
        assert(index < m_size);
        return m_data[(m_tail + index) % N];
    }

    const T& operator[](size_t index) const {
        assert(index < m_size);
        return m_data[(m_tail + index) % N];
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
