#pragma once

template <size_t N> struct fixed_string {
  char buffer[N + 1] = {};
  size_t length = 0;

  constexpr size_t size() const { return length; }
  constexpr size_t capacity() const { return N; }

  bool append(const char *s) {
    size_t i = 0;
    while (s[i]) {
      if (length >= N)
        return false;
      buffer[length++] = s[i++];
    }
    return true;
  }

  char &operator[](size_t i) { return buffer[i]; }
  const char &operator[](size_t i) const { return buffer[i]; }

  char *c_str() {
    buffer[length] = '\0';
    return buffer;
  }

  const char *c_str() const { return buffer; }
};
