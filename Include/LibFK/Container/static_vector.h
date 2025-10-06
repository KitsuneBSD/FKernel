#pragma once

#include <LibC/assert.h>
#include <LibC/stddef.h>
#include <LibFK/Traits/type_traits.h>

/*
 * @brief Fixed-capacity vector with stack allocation.
 *
 * Provides a simple vector-like container with a compile-time
 * fixed capacity. Does not allocate memory dynamically.
 *
 * @tparam T Type of elements
 * @tparam N Maximum number of elements
 */
template <typename T, size_t N>
struct static_vector
{
    T data[N];
    size_t count = 0;

    constexpr size_t size() const { return count; }
    constexpr size_t capacity() const { return N; }

    static void destroy(T *ptr)
    {
        ptr->~T();
    }

    // push_back for move-only types
    bool push_back(T &&value)
    {
        if (count >= N)
            return false;
        data[count++] = move(value);
        return true;
    }

    // optional: push_back for lvalues (makes a copy if possible)
    bool push_back(const T &value)
    {
        if (count >= N)
            return false;
        data[count++] = T(value); // copy or move ctor
        return true;
    }

    T &operator[](size_t i) { return data[i]; }
    const T &operator[](size_t i) const { return data[i]; }

    T *begin() { return data; }
    T *end() { return data + count; }
    const T *begin() const { return data; }
    const T *end() const { return data + count; }

    void erase(size_t index)
    {
        if (index >= count)
            return;
        for (size_t i = index; i < count - 1; ++i)
            data[i] = move(data[i + 1]);
        --count;
    }

    T &back()
    {
        ASSERT(count > 0);
        return data[count - 1];
    }

    const T &back() const
    {
        ASSERT(count > 0);
        return data[count - 1];
    }

    // NEW: access first element (complemento útil)
    T &front()
    {
        ASSERT(count > 0);
        return data[0];
    }

    const T &front() const
    {
        ASSERT(count > 0);
        return data[0];
    }

    void clear()
    {
        for (size_t i = 0; i < count; ++i)
            destroy(&data[i]);
        count = 0;
    }
};

template <typename T>
constexpr T &&move(T &obj) noexcept
{
    return static_cast<T &&>(obj);
}