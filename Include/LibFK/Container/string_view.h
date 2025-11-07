#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h> // For strlen, memcmp

/**
 * @brief Non-owning view over a constant character sequence.
 *
 * Similar to std::string_view, this class provides a lightweight, non-owning
 * reference to a contiguous sequence of characters. It does not manage the
 * lifetime of the underlying character data.
 */
class StringView {
public:
    using value_type = char;
    using size_type = size_t;
    using difference_type = ptrdiff_t;
    using const_pointer = const char*;
    using const_reference = const char&;
    using const_iterator = const char*;

    /**
     * @brief Default constructor. Creates an empty StringView.
     */
    constexpr StringView() noexcept : m_data(nullptr), m_size(0) {}

    /**
     * @brief Construct a StringView from a C-style string.
     * @param s Null-terminated C-style string.
     */
    constexpr StringView(const char* s) noexcept
        : m_data(s), m_size(s ? strlen(s) : 0) {}

    /**
     * @brief Construct a StringView from a pointer and a length.
     * @param data Pointer to the character sequence.
     * @param size Length of the character sequence.
     */
    constexpr StringView(const char* data, size_t size) noexcept
        : m_data(data), m_size(size) {}

    /**
     * @brief Get the size of the view.
     * @return Number of characters in the view.
     */
    constexpr size_type size() const noexcept { return m_size; }

    /**
     * @brief Get the length of the view.
     * @return Number of characters in the view.
     */
    constexpr size_type length() const noexcept { return m_size; }

    /**
     * @brief Check if the view is empty.
     * @return True if the view is empty, false otherwise.
     */
    constexpr bool empty() const noexcept { return m_size == 0; }

    /**
     * @brief Get a pointer to the beginning of the character sequence.
     * @return Const pointer to the character sequence.
     */
    constexpr const_pointer data() const noexcept { return m_data; }

    /**
     * @brief Access character at a specific index.
     * @param pos Index of the character.
     * @return Const reference to the character at 'pos'.
     */
    constexpr const_reference operator[](size_type pos) const noexcept { return m_data[pos]; }

    /**
     * @brief Get an iterator to the beginning of the view.
     * @return Const iterator to the beginning.
     */
    constexpr const_iterator begin() const noexcept { return m_data; }

    /**
     * @brief Get an iterator to the end of the view.
     * @return Const iterator to the end.
     */
    constexpr const_iterator end() const noexcept { return m_data + m_size; }

    /**
     * @brief Get an iterator to the beginning of the view.
     * @return Const iterator to the beginning.
     */
    constexpr const_iterator cbegin() const noexcept { return m_data; }

    /**
     * @brief Get an iterator to the end of the view.
     * @return Const iterator to the end.
     */
    constexpr const_iterator cend() const noexcept { return m_data + m_size; }

    /**
     * @brief Compare two StringView objects.
     * @param other The other StringView to compare with.
     * @return An integer less than, equal to, or greater than zero if this
     *         StringView is found, respectively, to be less than, to match,
     *         or to be greater than the 'other' StringView.
     */
    constexpr int compare(StringView other) const noexcept {
        size_type len = m_size < other.m_size ? m_size : other.m_size;
        int result = memcmp(m_data, other.m_data, len);
        if (result == 0) {
            if (m_size < other.m_size) return -1;
            if (m_size > other.m_size) return 1;
        }
        return result;
    }

    /**
     * @brief Check for equality with another StringView.
     * @param other The other StringView to compare with.
     * @return True if the views are equal, false otherwise.
     */
    constexpr bool operator==(StringView other) const noexcept {
        return m_size == other.m_size && compare(other) == 0;
    }

    /**
     * @brief Check for inequality with another StringView.
     * @param other The other StringView to compare with.
     * @return True if the views are not equal, false otherwise.
     */
    constexpr bool operator!=(StringView other) const noexcept {
        return !(*this == other);
    }

    /**
     * @brief Check if this StringView is less than another.
     * @param other The other StringView to compare with.
     * @return True if this StringView is lexicographically less than 'other'.
     */
    constexpr bool operator<(StringView other) const noexcept {
        return compare(other) < 0;
    }

    /**
     * @brief Check if this StringView is less than or equal to another.
     * @param other The other StringView to compare with.
     * @return True if this StringView is lexicographically less than or equal to 'other'.
     */
    constexpr bool operator<=(StringView other) const noexcept {
        return compare(other) <= 0;
    }

    /**
     * @brief Check if this StringView is greater than another.
     * @param other The other StringView to compare with.
     * @return True if this StringView is lexicographically greater than 'other'.
     */
    constexpr bool operator>(StringView other) const noexcept {
        return compare(other) > 0;
    }

    /**
     * @brief Check if this StringView is greater than or equal to another.
     * @param other The other StringView to compare with.
     * @return True if this StringView is lexicographically greater than or equal to 'other'.
     */
    constexpr bool operator>=(StringView other) const noexcept {
        return compare(other) >= 0;
    }

private:
    const char* m_data;
    size_type m_size;
};
