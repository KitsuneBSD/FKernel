#include <LibC/assert.h>
#include <LibC/limits.h>
#include <LibC/string.h>

// Helper to skip initial delimiters
static char *_strtok_skip_delimiters(char *s, size_t *str_len,
                                     const char *delim) {
  ASSERT(s != NULL);
  ASSERT(str_len != NULL);
  ASSERT(*str_len < SIZE_MAX); // Ensure string length is not excessively large
  ASSERT(delim != NULL);

  size_t current_pos = 0;
  size_t delim_len = strlen(delim); // Fixed bound for strlen
  ASSERT(delim_len <
         SIZE_MAX); // Ensure delimiter length is not excessively large

  while (current_pos < *str_len && strnchr(delim, s[current_pos], delim_len)) {
    current_pos++;
  }
  s += current_pos;
  *str_len -= current_pos;
  return s;
}

// Helper to find the end of a token
static size_t _strtok_find_token_end(const char *s, size_t str_len,
                                     const char *delim) {
  ASSERT(s != NULL);
  ASSERT(delim != NULL);
  ASSERT(str_len < SIZE_MAX); // Ensure string length is not excessively large

  size_t current_pos = 0;
  size_t delim_len = strlen(delim); // Fixed bound for strlen
  ASSERT(delim_len <
         SIZE_MAX); // Ensure delimiter length is not excessively large

  while (current_pos < str_len && !strnchr(delim, s[current_pos], delim_len)) {
    current_pos++;
  }
  return current_pos;
}

char *strtok_r(char *str, const char *delim, char **saveptr) {
  ASSERT(delim != NULL);
  ASSERT(saveptr != NULL);
  // The `str` parameter can be NULL for subsequent calls, so no ASSERT(str !=
  // NULL) here.

  char *s_current =
      str; // Use a distinct variable to indicate current processing position
  size_t s_len = 0;

  if (s_current) {
    s_len = strlen(s_current);
    ASSERT(s_len < SIZE_MAX); // Ensure string length is not excessively large
  } else {
    s_current = *saveptr;
    if (!s_current) { // If saveptr points to NULL, no more tokens
      *saveptr = NULL;
      return NULL;
    }
    ASSERT(s_current !=
           NULL); // Ensure s_current is not null if we are continuing
    s_len = strlen(s_current);
    ASSERT(s_len < SIZE_MAX); // Ensure string length is not excessively large
  }

  s_current = _strtok_skip_delimiters(s_current, &s_len, delim);

  if (s_len == 0) {
    *saveptr = NULL;
    return NULL;
  }

  char *token_start = s_current;
  size_t token_end_pos = _strtok_find_token_end(token_start, s_len, delim);

  if (token_end_pos < s_len) { // Delimiter found
    token_start[token_end_pos] = '\0';
    *saveptr = token_start + token_end_pos + 1;
  } else { // End of string
    *saveptr = NULL;
  }

  return token_start;
}
