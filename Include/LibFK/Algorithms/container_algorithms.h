#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace algorithms {

// Returns the index of the first element for which pred(element) is true,
// or `size` if none found. Works on any contiguous array.
template <typename T, typename Predicate>
size_t find_if(const T* data, size_t size, Predicate pred) {
    for (size_t i = 0; i < size; ++i) {
        if (pred(data[i])) return i;
    }
    return size;
}

// Removes the element at `index` from an array by swapping it with the last
// element and decrementing `size`. O(1), does not preserve order.
template <typename T>
void swap_remove(T* data, size_t& size, size_t index) {
    if (size == 0 || index >= size) return;
    if (index < size - 1) data[index] = data[size - 1];
    --size;
}

// Removes the first element for which pred returns true (swap-with-last).
// Returns true if an element was removed.
template <typename T, typename Predicate>
bool find_and_remove(T* data, size_t& size, Predicate pred) {
    size_t idx = find_if(data, size, pred);
    if (idx == size) return false;
    swap_remove(data, size, idx);
    return true;
}

// Dequeue `count` bytes from the front of a Vector-like buffer by shifting
// remaining bytes left. Caller must ensure count <= buf.size().
template <typename Buffer>
void dequeue_front(Buffer& buf, size_t count) {
    size_t remaining = buf.size() - count;
    for (size_t i = 0; i < remaining; ++i)
        buf[i] = buf[i + count];
    for (size_t i = 0; i < count; ++i)
        buf.pop_back();
}

// Insert `item` into `container` only if no existing element satisfies `equal`.
// Returns true if the item was inserted, false if a duplicate was found.
template <typename Container, typename T, typename Equal>
bool insert_if_absent(Container& container, const T& item, Equal equal) {
    for (const auto& existing : container) {
        if (equal(existing, item)) return false;
    }
    container.push_back(item);
    return true;
}

} // namespace algorithms
} // namespace fk
