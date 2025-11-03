#pragma once

#include <LibFK/Algorithms/djb2.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#endif

/**
 * @brief Template traits for generic types.
 *
 * Provides auxiliary functions such as hashing and dumping
 * for different data types.
 *
 * @tparam T Data type
 */
template <typename T> struct Traits {};

/**
 * @brief Traits specialization for int
 */
template <> struct Traits<int> {
  /**
   * @brief Computes the hash of an integer using DJB2.
   *
   * @param i Integer value to hash
   * @return Hash value
   */
  static unsigned hash(int i) { return djb2(&i, sizeof(i)); }

  /**
   * @brief Prints the integer value.
   *
   * @param i Integer value to print
   */
  static void dump(int i) { kprintf("%d", i); }
};

/**
 * @brief Traits specialization for unsigned int
 */
template <> struct Traits<unsigned> {
  /**
   * @brief Computes the hash of an unsigned integer using DJB2.
   *
   * @param i Unsigned integer value to hash
   * @return Hash value
   */
  static unsigned hash(unsigned i) { return djb2(&i, sizeof(i)); }

  /**
   * @brief Prints the unsigned integer value.
   *
   * @param i Unsigned integer value to print
   */
  static void dump(unsigned i) { kprintf("%u", i); }
};
