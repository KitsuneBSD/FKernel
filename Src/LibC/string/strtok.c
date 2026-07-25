#include <LibC/limits.h>
#include <LibC/string.h>

static char *_strtok_skip_delimiters(char *s, size_t *str_len,
                                     const char *delim) {
  size_t current_pos = 0;
  size_t delim_len = strlen(delim);

  while (current_pos < *str_len && strnchr(delim, s[current_pos], delim_len)) {
    current_pos++;
  }
  s += current_pos;
  *str_len -= current_pos;
  return s;
}

static size_t _strtok_find_token_end(const char *s, size_t str_len,
                                     const char *delim) {
  size_t current_pos = 0;
  size_t delim_len = strlen(delim);

  while (current_pos < str_len && !strnchr(delim, s[current_pos], delim_len)) {
    current_pos++;
  }
  return current_pos;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
  if (!delim || !saveptr) return NULL;

  char *s_current = str;
  size_t s_len = 0;

  if (s_current) {
    s_len = strlen(s_current);
  } else {
    s_current = *saveptr;
    if (!s_current) {
      *saveptr = NULL;
      return NULL;
    }
    s_len = strlen(s_current);
  }

  s_current = _strtok_skip_delimiters(s_current, &s_len, delim);

  if (s_len == 0) {
    *saveptr = NULL;
    return NULL;
  }

  char *token_start = s_current;
  size_t token_end_pos = _strtok_find_token_end(token_start, s_len, delim);

  if (token_end_pos < s_len) {
    token_start[token_end_pos] = '\0';
    *saveptr = token_start + token_end_pos + 1;
  } else {
    *saveptr = NULL;
  }

  return token_start;
}

static char *s_strtok_saveptr = ((void *)0);

char *strtok(char *str, const char *delim) {
  return strtok_r(str, delim, &s_strtok_saveptr);
}
