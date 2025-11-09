#pragma once

#include <LibC/stdint.h>
#include <LibC/stddef.h>

#ifdef __cplusplus
extern "C"
{
#endif

  /* ==========================================
     CONVERSÕES DE NÚMEROS
     ========================================== */

  /* Converte número sem sinal para string (base 2-16) */
  static inline void itoa_unsigned(uint64_t value, char *buf, int base, int uppercase)
  {
    char tmp[65];
    const char *digits = uppercase ? "0123456789ABCDEF" : "0123456789abcdef";
    int i = 0;

    if (value == 0)
    {
      tmp[i++] = '0';
    }
    else
    {
      while (value)
      {
        tmp[i++] = digits[value % base];
        value /= base;
      }
    }

    int j = 0;
    while (i > 0)
    {
      buf[j++] = tmp[--i];
    }
    buf[j] = '\0';
  }

  static inline void itoa_signed(int64_t value, char *buf, int base)
  {
    if (value < 0)
    {
      *buf++ = '-';
      itoa_unsigned((uint64_t)(-value), buf, base, 0);
    }
    else
    {
      itoa_unsigned((uint64_t)value, buf, base, 0);
    }
  }

  static inline void itoa_size(size_t value, char *buf, int base)
  {
    itoa_unsigned((uint64_t)value, buf, base, 0);
  }

  static char *strtok_saveptr = NULL;

  /**
   * @brief Digits used for number-to-string conversions (base up to 16).
   */
  static const char digits[] = "0123456789abcdef";

  /**
   * @brief Copy memory region, handling overlapping areas safely.
   *
   * @param dest Destination buffer.
   * @param src  Source buffer.
   * @param n    Number of bytes to copy.
   * @return Pointer to @p dest.
   */
  void *memmove(void *dest, const void *src, size_t n);

  /**
   * @brief Copy memory region (undefined behavior if regions overlap).
   *
   * @param dest Destination buffer.
   * @param src  Source buffer.
   * @param n    Number of bytes to copy.
   * @return Pointer to @p dest.
   */
  void *memcpy(void *dest, const void *src, size_t n);

  /**
   * @brief Fill a memory region with a constant byte value.
   *
   * @param s Pointer to the memory region.
   * @param c Byte value to set.
   * @param n Number of bytes to set.
   * @return Pointer to @p s.
   */
  void *memset(void *s, int c, size_t n);

  /**
   * @brief Compare two memory regions.
   *
   * @param s1 Pointer to the first memory region.
   * @param s2 Pointer to the second memory region.
   * @param n  Number of bytes to compare.
   * @return < 0 if @p s1 < @p s2, 0 if equal, > 0 if @p s1 > @p s2.
   */
  int memcmp(const void *s1, const void *s2, size_t n);

  /**
   * @brief Compute the length of a null-terminated string.
   *
   * @param s Pointer to the string.
   * @return Length of the string, excluding the null terminator.
   */
  size_t strlen(const char *s);

  /**
   * @brief Copy a null-terminated string.
   *
   * @param dest Destination buffer.
   * @param src  Source string.
   * @return Pointer to @p dest.
   */
  char *strcpy(char *dest, const char *src);

  /**
   * @brief Compare two null-terminated strings.
   *
   * @param s1 Pointer to the first string.
   * @param s2 Pointer to the second string.
   * @return < 0 if @p s1 < @p s2, 0 if equal, > 0 if @p s1 > @p s2.
   */
  int strcmp(const char *s1, const char *s2);

  /**
   * @brief Locate the first occurrence of a character in a string.
   *
   * @param s Pointer to the string.
   * @param c Character to search for (interpreted as unsigned char).
   * @return Pointer to the first occurrence of @p c, or NULL if not found.
   */
  char *strchr(const char *s, int c);

  /**
   * @brief Concatenate two null-terminated strings.
   *
   * @param dest Destination buffer (must be large enough).
   * @param src  Source string.
   * @return Pointer to @p dest.
   */
  char *strcat(char *dest, const char *src);

  /**
   * @brief Copy at most @p n characters from one string to another.
   *
   * @param dest Destination buffer.
   * @param src  Source string.
   * @param n    Maximum number of characters to copy.
   * @return Pointer to @p dest.
   *
   * @note If @p src is shorter than @p n, the remainder of @p dest is padded with
   * nulls.
   */
  char *strncpy(char *dest, const char *src, size_t n);

  /**
   * @brief Compute the length of a string up to a maximum.
   *
   * @param s      Pointer to the string.
   * @param maxlen Maximum number of characters to examine.
   * @return Length of the string, up to @p maxlen.
   */
  size_t strnlen(const char *s, size_t maxlen);

  /**
   * @brief Convert an integer to a string in the specified base.
   *
   * @param val  Integer value.
   * @param buf  Buffer to store the result (must be large enough).
   * @param base Base for conversion (2–16).
   * @return Number of characters written (excluding null terminator).
   */
  int itoa(int val, char *buf, int base);

  /**
   * @brief Convert a string to an integer.
   *
   * @param str Null-terminated string.
   * @return Integer value represented by @p str.
   */
  int atoi(const char *str);

  /**
   * @brief Convert a string to a long integer.
   *
   * @param str Null-terminated string.
   * @return Long integer value represented by @p str.
   */
  long stol(const char *str);

  /**
   * @brief Convert an unsigned long integer to a string in the specified base.
   *
   * @param value  Unsigned long value.
   * @param buffer Buffer to store the result (must be large enough).
   * @param base   Base for conversion (2–16).
   * @return Number of characters written (excluding null terminator).
   */
  size_t ultoa(unsigned long value, char *buffer, unsigned int base);

  /**
   * @brief Locate the last occurrence of a character in a string.
   *
   * @param s Pointer to the string.
   * @param c Character to search for (interpreted as unsigned char).
   * @return Pointer to the last occurrence of @p c, or NULL if not found.
   */
  char *strrchr(const char *s, int c);

  /**
   * @brief Tokenize a string using delimiters.
   *
   * @param str     String to tokenize (pass NULL to continue tokenizing previous string).
   * @param delim   Null-terminated string with delimiter characters.
   * @return Pointer to next token, or NULL if no more tokens.
   */
  char *strtok(char *str, const char *delim);

  /**
 * @brief Compare two strings up to a specified number of characters.
 *
 * Compares at most @p n characters of the null-terminated strings @p s1 and @p s2.
 * The comparison is done using unsigned characters.
 *
 * @param s1 Pointer to the first string to compare.
 * @param s2 Pointer to the second string to compare.
 * @param n Maximum number of characters to compare.
 * @return
 *   - < 0 if @p s1 is lexicographically less than @p s2  
 *   - 0 if the compared parts of both strings are equal  
 *   - > 0 if @p s1 is lexicographically greater than @p s2
 *
 * @note
 * If @p n is zero, the function returns 0 immediately.
 */
  int strncmp(const char *s1, const char *s2, size_t n);

#ifdef __cplusplus
}
#endif