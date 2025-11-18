#include <LibFK/Container/string.h>
#include <LibC/string.h>
#include <LibFK/new.h>
#include <LibC/assert.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h> // For fk::types::move

namespace fk {
namespace text {

String::String() : m_data(nullptr), m_metadata({0, 0}) { // Initialize metadata pair with 0 length and 0 capacity
    auto res = ensure_capacity(16);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory for empty string.");
        // m_data is already nullptr by default for OwnPtr
        m_metadata = {0, 0};
        return;
    }
    // If ensure_capacity succeeded, m_data is now valid.
    m_data.ptr()[0] = '\0'; // Use ptr() to access the raw pointer
}

String::String(const char* s) : m_data(nullptr), m_metadata({0, 0}) { // Initialize metadata pair
    if (!s) {
        auto res = ensure_capacity(16);
        if (res.is_error()) {
            fk::algorithms::kerror("String", "Failed to allocate memory for empty string.");
            m_metadata = {0, 0};
            return;
        }
        m_data.ptr()[0] = '\0'; // Use ptr()
        return;
    }
    m_metadata.first = strlen(s); // Use m_metadata.first for length
    auto res = ensure_capacity(m_metadata.first + 1);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory for string initialization.");
        m_metadata = {0, 0};
        return;
    }
    memcpy(m_data.ptr(), s, m_metadata.first); // Use ptr()
    m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
}

String::String(const String& other) : m_data(nullptr), m_metadata({other.m_metadata.first, 0}) { // Initialize metadata pair with other's length
    auto res = ensure_capacity(m_metadata.first + 1);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory for string copy.");
        m_metadata = {0, 0};
        return;
    }
    // If ensure_capacity succeeded, m_data is now valid.
    memcpy(m_data.ptr(), other.m_data.ptr(), m_metadata.first); // Use ptr() for both
    m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
}

String::String(String&& other) noexcept : m_data(fk::types::move(other.m_data)), m_metadata(other.m_metadata) { // Move metadata pair
    // OwnPtr's move constructor handles nulling out the source.
    other.m_metadata = {0, 0}; // Reset other's metadata
}

String::~String() {
    // OwnPtr handles deletion, no need for manual delete[] m_data;
}

String& String::operator=(const String& other) {
    if (this == &other) {
        return *this;
    }
    m_metadata.first = other.m_metadata.first; // Update length
    auto res = ensure_capacity(m_metadata.first + 1);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory for string assignment.");
        m_data = nullptr; // Reset OwnPtr on error
        m_metadata = {0, 0};
        return *this;
    }
    // If ensure_capacity succeeded, m_data is now valid.
    memcpy(m_data.ptr(), other.m_data.ptr(), m_metadata.first); // Use ptr() for both
    m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
    return *this;
}

String& String::operator=(String&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    m_data = fk::types::move(other.m_data); // Move OwnPtr
    m_metadata = other.m_metadata; // Move metadata pair
    // OwnPtr's move assignment handles nulling out the source.
    other.m_metadata = {0, 0}; // Reset other's metadata
    return *this;
}

String& String::operator=(const char* s) {
    if (!s) {
        m_metadata.first = 0; // Reset length
        if (m_metadata.second > 0) { // Use m_metadata.second for capacity
            m_data.ptr()[0] = '\0'; // Use ptr()
        } else {
            // If capacity is 0, ensure at least 1 byte for null terminator
            auto res = ensure_capacity(1);
            if (res.is_error()) {
                fk::algorithms::kerror("String", "Failed to allocate memory for empty string assignment.");
                m_data = nullptr; // Reset OwnPtr on error
                m_metadata = {0, 0};
            } else {
                m_data.ptr()[0] = '\0'; // Use ptr()
            }
        }
        return *this;
    }
    m_metadata.first = strlen(s); // Update length
    auto res = ensure_capacity(m_metadata.first + 1);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory for string assignment.");
        m_data = nullptr; // Reset OwnPtr on error
        m_metadata = {0, 0};
        return *this;
    }
    // If ensure_capacity succeeded, m_data is now valid.
    memcpy(m_data.ptr(), s, m_metadata.first); // Use ptr()
    m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
    return *this;
}

void String::clear() {
    m_metadata.first = 0; // Reset length
    if (m_data) {
        m_data.ptr()[0] = '\0'; // Use ptr()
    }
}

fk::core::Result<void, fk::core::Error> String::reserve(size_t new_cap) {
    if (new_cap > m_metadata.second) { // Use m_metadata.second for capacity
        return ensure_capacity(new_cap);
    }
    return fk::core::Result<void>(); // Already has enough capacity, success
}

String& String::append(const char* s) {
    if (!s) return *this;
    size_t s_len = strlen(s);
    if (s_len == 0) return *this;
    auto res = ensure_capacity(m_metadata.first + s_len + 1);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory during append.");
        // String remains in its current state, but append failed.
        return *this;
    }
    // If ensure_capacity succeeded, m_data is now valid.
    memcpy(m_data.ptr() + m_metadata.first, s, s_len); // Use ptr()
    m_metadata.first += s_len; // Update length
    m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
    return *this;
}

String& String::append(const String& str) {
    if (str.is_empty()) return *this;
    auto res = ensure_capacity(m_metadata.first + str.m_metadata.first + 1);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory during append.");
        // String remains in its current state, but append failed.
        return *this;
    }
    // If ensure_capacity succeeded, m_data is now valid.
    memcpy(m_data.ptr() + m_metadata.first, str.m_data.ptr(), str.m_metadata.first); // Use ptr() for both
    m_metadata.first += str.m_metadata.first; // Update length
    m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
    return *this;
}

void String::push_back(char c) {
    auto res = ensure_capacity(m_metadata.first + 2);
    if (res.is_error()) {
        fk::algorithms::kerror("String", "Failed to allocate memory during push_back.");
        // String remains in its current state, but push_back failed.
        return;
    }
    // If ensure_capacity succeeded, m_data is now valid.
    m_data.ptr()[m_metadata.first++] = c; // Use ptr()
    m_data.ptr()[m_metadata.first] = '\0'; // Use ptr()
}

char& String::operator[](size_t index) {
    ASSERT(index < m_metadata.first); // Use m_metadata.first for length
    return m_data.ptr()[index]; // Use ptr()
}

const char& String::operator[](size_t index) const {
    ASSERT(index < m_metadata.first); // Use m_metadata.first for length
    return m_data.ptr()[index]; // Use ptr()
}

fk::core::Result<void, fk::core::Error> String::ensure_capacity(size_t min_cap) {
    if (m_metadata.second >= min_cap) { // Use m_metadata.second for capacity
        return fk::core::Result<void>(); // Already has enough capacity, success
    }
    size_t new_cap = m_metadata.second == 0 ? 16 : m_metadata.second * 2;
    if (new_cap < min_cap) {
        new_cap = min_cap;
    }
    char* new_data = new char[new_cap];
    if (!new_data) { // Check for allocation failure
        // Log error and return OutOfMemory
        fk::algorithms::kerror("String", "Failed to allocate memory in ensure_capacity.");
        return fk::core::Error::OutOfMemory;
    }
    if (m_data) {
        memcpy(new_data, m_data.ptr(), m_metadata.first + 1); // Copy existing data including null terminator, use ptr()
        delete[] m_data.ptr(); // Use ptr() for delete[]
    }
    m_data = new_data; // OwnPtr assignment should handle ownership transfer
    m_metadata.second = new_cap; // Update capacity
    return fk::core::Result<void>(); // Success
}

} // namespace text
} // namespace fk
