#include <LibFK/Container/string.h>
#include <LibC/string.h>
#include <LibFK/new.h>
#include <LibC/assert.h>

String::String() : m_data(nullptr), m_length(0), m_capacity(0) {
    ensure_capacity(16);
    m_data[0] = '\0';
}

String::String(const char* s) : m_data(nullptr), m_length(0), m_capacity(0) {
    if (!s) {
        ensure_capacity(16);
        m_data[0] = '\0';
        return;
    }
    m_length = strlen(s);
    ensure_capacity(m_length + 1);
    memcpy(m_data, s, m_length);
    m_data[m_length] = '\0';
}

String::String(const String& other) : m_data(nullptr), m_length(other.m_length), m_capacity(0) {
    ensure_capacity(m_length + 1);
    memcpy(m_data, other.m_data, m_length);
    m_data[m_length] = '\0';
}

String::String(String&& other) noexcept : m_data(other.m_data), m_length(other.m_length), m_capacity(other.m_capacity) {
    other.m_data = nullptr;
    other.m_length = 0;
    other.m_capacity = 0;
}

String::~String() {
    delete[] m_data;
}

String& String::operator=(const String& other) {
    if (this == &other) {
        return *this;
    }
    m_length = other.m_length;
    ensure_capacity(m_length + 1);
    memcpy(m_data, other.m_data, m_length);
    m_data[m_length] = '\0';
    return *this;
}

String& String::operator=(String&& other) noexcept {
    if (this == &other) {
        return *this;
    }
    delete[] m_data;
    m_data = other.m_data;
    m_length = other.m_length;
    m_capacity = other.m_capacity;
    other.m_data = nullptr;
    other.m_length = 0;
    other.m_capacity = 0;
    return *this;
}

String& String::operator=(const char* s) {
    if (!s) {
        m_length = 0;
        if (m_capacity > 0) {
            m_data[0] = '\0';
        }
        return *this;
    }
    m_length = strlen(s);
    ensure_capacity(m_length + 1);
    memcpy(m_data, s, m_length);
    m_data[m_length] = '\0';
    return *this;
}

void String::clear() {
    m_length = 0;
    if (m_data) {
        m_data[0] = '\0';
    }
}

void String::reserve(size_t new_cap) {
    if (new_cap > m_capacity) {
        ensure_capacity(new_cap);
    }
}

String& String::append(const char* s) {
    if (!s) return *this;
    size_t s_len = strlen(s);
    if (s_len == 0) return *this;
    ensure_capacity(m_length + s_len + 1);
    memcpy(m_data + m_length, s, s_len);
    m_length += s_len;
    m_data[m_length] = '\0';
    return *this;
}

String& String::append(const String& str) {
    if (str.is_empty()) return *this;
    ensure_capacity(m_length + str.m_length + 1);
    memcpy(m_data + m_length, str.m_data, str.m_length);
    m_length += str.m_length;
    m_data[m_length] = '\0';
    return *this;
}

void String::push_back(char c) {
    ensure_capacity(m_length + 2);
    m_data[m_length++] = c;
    m_data[m_length] = '\0';
}

char& String::operator[](size_t index) {
    ASSERT(index < m_length);
    return m_data[index];
}

const char& String::operator[](size_t index) const {
    ASSERT(index < m_length);
    return m_data[index];
}

void String::ensure_capacity(size_t min_cap) {
    if (m_capacity >= min_cap) {
        return;
    }
    size_t new_cap = m_capacity == 0 ? 16 : m_capacity * 2;
    if (new_cap < min_cap) {
        new_cap = min_cap;
    }
    char* new_data = new char[new_cap];
    if (m_data) {
        memcpy(new_data, m_data, m_length + 1);
        delete[] m_data;
    }
    m_data = new_data;
    m_capacity = new_cap;
}
