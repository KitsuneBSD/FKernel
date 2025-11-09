#pragma once

#include <LibC/stddef.h>
#include <LibC/string.h> // For strcmp, strlen

class String {
public:
    using iterator = char*;
    using const_iterator = const char*;

    String();
    String(const char* s);
    String(const String& other);
    String(String&& other) noexcept;

    ~String();

    String& operator=(const String& other);
    String& operator=(String&& other) noexcept;
    String& operator=(const char* s);

    size_t length() const { return m_length; }
    size_t size() const { return m_length; }
    size_t capacity() const { return m_capacity; }
    bool is_empty() const { return m_length == 0; }
    const char* c_str() const { return m_data ? m_data : ""; }
    char* data() { return m_data; }
    const char* data() const { return m_data; }

    void clear();
    void reserve(size_t new_cap);

    String& append(const char* s);
    String& append(const String& str);
    void push_back(char c);

    String& operator+=(const char* s) { return append(s); }
    String& operator+=(const String& str) { return append(str); }

    char& operator[](size_t index);
    const char& operator[](size_t index) const;

    iterator begin() { return m_data; }
    iterator end() { return m_data + m_length; }
    const_iterator begin() const { return m_data; }
    const_iterator end() const { return m_data + m_length; }
    const_iterator cbegin() const { return m_data; }
    const_iterator cend() const { return m_data + m_length; }

private:
    void ensure_capacity(size_t min_cap);

    char* m_data;
    size_t m_length;
    size_t m_capacity;
};

inline String operator+(const String& lhs, const String& rhs) {
    String result;
    result.reserve(lhs.length() + rhs.length() + 1);
    result.append(lhs);
    result.append(rhs);
    return result;
}

inline String operator+(const String& lhs, const char* rhs) {
    if (!rhs) return lhs;
    String result;
    result.reserve(lhs.length() + strlen(rhs) + 1);
    result.append(lhs);
    result.append(rhs);
    return result;
}

inline String operator+(const char* lhs, const String& rhs) {
    if (!lhs) return rhs;
    String result;
    result.reserve(strlen(lhs) + rhs.length() + 1);
    result.append(lhs);
    result.append(rhs);
    return result;
}

inline bool operator==(const String& lhs, const String& rhs) {
    return lhs.length() == rhs.length() && strcmp(lhs.c_str(), rhs.c_str()) == 0;
}

inline bool operator==(const String& lhs, const char* rhs) {
    if (!rhs) return lhs.is_empty();
    return strcmp(lhs.c_str(), rhs) == 0;
}

inline bool operator==(const char* lhs, const String& rhs) {
    if (!lhs) return rhs.is_empty();
    return strcmp(lhs, rhs.c_str()) == 0;
}

inline bool operator!=(const String& lhs, const String& rhs) {
    return !(lhs == rhs);
}

inline bool operator!=(const String& lhs, const char* rhs) {
    return !(lhs == rhs);
}

inline bool operator!=(const char* lhs, const String& rhs) {
    return !(lhs == rhs);
}
