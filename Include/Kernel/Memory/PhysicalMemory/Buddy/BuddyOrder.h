#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief The minimum order for buddy memory allocation.
 */
static constexpr size_t MIN_ORDER = 12;

/**
 * @brief The page size in bytes.
 */
static constexpr size_t BUDDY_PAGE_SIZE = 1ull << MIN_ORDER;

/**
 * @brief The maximum order for buddy memory allocation.
 */
static constexpr size_t MAX_ORDER = 21;

/**
 * @brief The number of orders available in the buddy memory allocator.
 */
static constexpr size_t NUM_ORDERS = MAX_ORDER - MIN_ORDER + 1;

/**
 * @brief Converts a buddy order to its corresponding size in bytes.
 */
constexpr size_t order_to_size(size_t order) { return 1ull << order; }

inline size_t size_to_order(size_t size) {
  size_t order = MIN_ORDER;

  while ((1ull << order) < size && order <= MAX_ORDER) {
    order++;
  }

  return order;
}
