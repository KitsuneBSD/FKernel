#pragma once

/**
 * @typedef va_list
 * @brief Type used for iterating arguments in functions with variable
 * arguments.
 *
 * This type is provided by the compiler as a builtin. It is used
 * together with the macros ::va_start, ::va_arg, ::va_end, and ::va_copy.
 */
typedef __builtin_va_list va_list;

/**
 * @def va_start(ap, last)
 * @brief Initialize a va_list for argument iteration.
 *
 * @param ap   The va_list to initialize.
 * @param last The name of the last fixed parameter before the variadic
 * arguments.
 */
#define va_start(ap, last) __builtin_va_start(ap, last)

/**
 * @def va_arg(ap, type)
 * @brief Retrieve the next argument from the va_list.
 *
 * @param ap   The va_list being iterated.
 * @param type The type of the next argument.
 * @return The next argument value, cast to @p type.
 */
#define va_arg(ap, type) __builtin_va_arg(ap, type)

/**
 * @def va_end(ap)
 * @brief Clean up a va_list after use.
 *
 * @param ap The va_list to finalize.
 */
#define va_end(ap) __builtin_va_end(ap)

/**
 * @def va_copy(dest, src)
 * @brief Copy the state of a va_list.
 *
 * @param dest The destination va_list.
 * @param src  The source va_list.
 */
#define va_copy(dest, src) __builtin_va_copy(dest, src)
