#pragma once

/**
 * @brief Marks a function that never returns.
 */
#define FK_NORETURN __attribute__((noreturn))

/**
 * @brief Packs a struct without automatic padding.
 */
#define FK_PACKED __attribute__((packed))

/**
 * @brief Marks a variable or parameter as intentionally unused.
 */
#define FK_UNUSED __attribute__((unused))

/**
 * @brief Hints the compiler that the condition is very likely.
 *
 * @param x Boolean expression.
 */
#define FK_LIKELY(x) __builtin_expect(!!(x), 1)

/**
 * @brief Hints the compiler that the condition is unlikely.
 *
 * @param x Boolean expression.
 */
#define FK_UNLIKELY(x) __builtin_expect(!!(x), 0)

/**
 * @brief Enforces a minimum alignment for a symbol.
 *
 * @param x Alignment in bytes.
 */
#define FK_ALIGN(x) __attribute__((aligned(x)))

/**
 * @brief Forces the compiler to always inline the function.
 */
#define FK_ALWAYS_INLINE inline __attribute__((always_inline))

/**
 * @brief Prevents the compiler from inlining the function.
 */
#define FK_NOINLINE __attribute__((noinline))

/**
 * @brief Disables optimizations for a specific function.
 *
 * Useful for debugging sensitive code paths or hardware-specific routines.
 */
#define FK_OPTNONE __attribute__((optnone))

/**
 * @brief Specifies that the given function parameters must not be null.
 *
 * @param ... 1-based indices of non-null parameters.
 */
#define FK_NONNULL(...) __attribute__((nonnull(__VA_ARGS__)))

/**
 * @brief Places a symbol into a specific binary section.
 *
 * @param x Section name, e.g. ".init", ".text", ".bss".
 */
#define FK_SECTION(x) __attribute__((section(x)))
