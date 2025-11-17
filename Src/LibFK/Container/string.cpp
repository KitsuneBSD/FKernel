#include <LibFK/Container/string.h>
#include <LibC/string.h>
#include <LibFK/new.h>
#include <LibC/assert.h>

String::String() : m_data(nullptr), m_metadata({0, 0}) { // Initialize metadata pair with 0 length and 0 capacity
    ensure_capacity(16);
    m_data[0] = '\0';
}

String::String(const char* s) : m_data(nullptr), m_metadata({0, 0}) { // Initialize metadata pair
    if (!s) {
        ensure_capacity(16);
        m_data[0] = '\0';
        return;
    }
    m_metadata.first = strlen(s); // Use m_metadata.first for length
    ensure_capacity(m_metadata.first + 1);
    memcpy(m_data, s, m_metadata.first);
    m_data[m_metadata.first] = '\0';
}

String::String(const String& other) : m_data(nullptr), m_metadata({other.m_metadata.first, 0}) { // Initialize metadata pair with other's length
    ensure_capacity(m_metadata.first + 1);
    memcpy(m_data, other.m_data, m_metadata.first);
    m_data[m_metadata.first] = '\0';
}

String::String(String&& other) noexcept : m_data(other.m_data), m_metadata(other.m_metadata) { // Move metadata pair
    other.m_data = nullptr;
    other.m_metadata = {0, 0}; // Reset other's metadata
}

String::~String() {
    delete[] m_data;
}

String& String::operator=(const String& other) {
    if (this == &other) {
        return *this;
    }
    m_metadata.first = other.m_metadata.first; // Update length
    ensure_capacity(m_metadata.first + 1);
    memcpy(m_data, other.m_data, m_metadata.first);
    m_data[m_metadata.first] = '\0';
    return *this;
}

String& String::operator=(String&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    delete[] m_data;
    m_data = other.m_data;
    m_metadata = other.m_metadata; // Move metadata pair
    other.m_data = nullptr;
    other.m_metadata = {0, 0}; // Reset other's metadata
    return *this;
}

String& String::operator=(const char* s) {
    if (!s) {
        m_metadata.first = 0; // Reset length
        if (m_metadata.second > 0) { // Use m_metadata.second for capacity
            m_data[0] = '\0';
        }
        return *this;
    }
    m_metadata.first = strlen(s); // Update length
    ensure_capacity(m_metadata.first + 1);
    memcpy(m_data, s, m_metadata.first);
    m_data[m_metadata.first] = '\0';
    return *this;
}

void String::clear() {
    m_metadata.first = 0; // Reset length
    if (m_data) {
        m_data[0] = '\0';
    }
}

void String::reserve(size_t new_cap) {
    if (new_cap > m_metadata.second) { // Use m_metadata.second for capacity
        ensure_capacity(new_cap);
    }
}

String& String::append(const char* s) {
    if (!s) return *this;
    size_t s_len = strlen(s);
    if (s_len == 0) return *this;
    ensure_capacity(m_metadata.first + s_len + 1);
    memcpy(m_data + m_metadata.first, s, s_len);
    m_metadata.first += s_len; // Update length
    m_data[m_metadata.first] = '\0';
    return *this;
}

String& String::append(const String& str) {
    if (str.is_empty()) return *this;
    ensure_capacity(m_metadata.first + str.m_metadata.first + 1);
    memcpy(m_data + m_metadata.first, str.m_data, str.m_metadata.first);
    m_metadata.first += str.m_metadata.first; // Update length
    m_data[m_metadata.first] = '\0';
    return *this;
}

void String::push_back(char c) {
    ensure_capacity(m_metadata.first + 2);
    m_data[m_metadata.first++] = c;
    m_data[m_metadata.first] = '\0';
}

char& String::operator[](size_t index) {
    ASSERT(index < m_metadata.first); // Use m_metadata.first for length
    return m_data[index];
}

const char& String::operator[](size_t index) const {
    ASSERT(index < m_metadata.first); // Use m_metadata.first for length
    return m_data[index];
}

void String::ensure_capacity(size_t min_cap) {
    if (m_metadata.second >= min_cap) { // Use m_metadata.second for capacity
        return;
    }
    size_t new_cap = m_metadata.second == 0 ? 16 : m_metadata.second * 2;
    if (new_cap < min_cap) {
        new_cap = min_cap;
    }
    char* new_data = new char[new_cap];
    if (m_data) {
        memcpy(new_data, m_data, m_metadata.first + 1); // Copy existing data including null terminator
        delete[] m_data;
    }
    m_data = new_data;
    m_metadata.second = new_cap; // Update capacity
}
