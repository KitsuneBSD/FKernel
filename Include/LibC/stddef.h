#pragma once

#include <LibC/stdint.h>

/**
 * @typedef size_t
 * @brief Unsigned integer type used to represent object sizes and array
 * indices.
 */
typedef __SIZE_TYPE__ size_t;

/**
 * @typedef ptrdiff_t
 * @brief Signed integer type capable of holding the difference between two
 * pointers.
 */
typedef __PTRDIFF_TYPE__ ptrdiff_t;

/**
 * @typedef ssize_t
 * @brief Signed integer type similar to ::size_t but allows negative values.
 */
typedef int64_t ssize_t;

#ifdef __cplusplus
/**
 * @def NULL
 * @brief Null pointer constant in C++.
 *
 * Defined as `nullptr` for type safety.
 */
#define NULL nullptr

/**
 * @typedef nullptr_t
 * @brief Type of the null pointer literal `nullptr`.
 */
using nullptr_t = decltype(nullptr);

#else
/**
 * @def NULL
 * @brief Null pointer constant in C.
 *
 * Defined as `(void *)0`.
 */
#define NULL ((void *)0)
#endif

/**
 * @def offsetof
 * @brief Compute the offset of a struct/union member in bytes.
 *
 * @param type   The structure type.
 * @param member The name of the member within the structure.
 * @return The offset of @p member within @p type, in bytes.
 */
#define offsetof(type, member) __builtin_offsetof(type, member)

/**
 * @def container_of
 * @brief Get a pointer to the parent structure from a pointer to one of its
 * members.
 *
 * @param ptr    Pointer to the member.
 * @param type   Type of the container structure.
 * @param member Name of the member within the structure.
 * @return Pointer to the container structure of type @p type.
 *
 * @note This is a common idiom used in low-level systems code to recover
 *       the enclosing object from a member pointer.
 */
#define container_of(ptr, type, member)                                        \
  ((type *)((char *)(ptr) - offsetof(type, member)))
