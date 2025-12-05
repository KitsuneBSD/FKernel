#pragma once

#include <LibFK/Algorithms/djb2.h>
#include <LibFK/Algorithms/log.h>
#ifdef __x86_64
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Algorithms/crc32.h>
#endif

namespace fk {
namespace traits {

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
  static unsigned hash(int i) {
#ifdef __x86_64
    return crc32(&i, sizeof(i));
#else
    return djb2(&i, sizeof(i));
#endif
  }

  /**
   * @brief Prints the integer value.
   *
   * @param i Integer value to print
   */
  static void dump(int i) { kdebug("TRAITS", "dump(int): %d", i); }
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
  static unsigned hash(unsigned i) {
#ifdef __x86_64
    return crc32(&i, sizeof(i));
#else
    return djb2(&i, sizeof(i));
#endif
  }

  /**
   * @brief Prints the unsigned integer value.
   *
   * @param i Unsigned integer value to print
   */
  static void dump(unsigned i) { kdebug("TRAITS", "dump(unsigned): %u", i); }
};

} // namespace traits
} // namespace fk