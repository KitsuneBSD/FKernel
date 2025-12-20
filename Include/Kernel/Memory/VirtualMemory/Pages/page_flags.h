#pragma once

#include <LibFK/Types/types.h>

/**
 * @brief Page table entry flags for x86_64 paging
 *
 * Used to control page properties such as presence, permissions, caching, and
 * execution.
 */
enum PageFlags : uint64_t {
  /// Page is present in memory
  Present = 1ULL << 0,

  /// Page is writable
  Writable = 1ULL << 1,

  /// User-mode code can access this page
  User = 1ULL << 2,

  /// Write-through caching enabled
  WriteThrough = 1ULL << 3,

  /// Cache is disabled for this page
  CacheDisabled = 1ULL << 4,

  /// Page has been accessed (set by CPU)
  Accessed = 1ULL << 5,

  /// Page has been written to (set by CPU)
  Dirty = 1ULL << 6,

  /// Huge page (2 MiB or 1 GiB)
  HugePage = 1ULL << 7,

  /// Global page (does not get invalidated in TLB on CR3 reload)
  Global = 1ULL << 8,

  /// No-execute (NX) bit, if supported
  ExecuteDisable = 1ULL << 63
};

inline PageFlags operator|(PageFlags a, PageFlags b) {
    return static_cast<PageFlags>(static_cast<uint64_t>(a) | static_cast<uint64_t>(b));
}

inline PageFlags operator&(PageFlags a, PageFlags b) {
    return static_cast<PageFlags>(static_cast<uint64_t>(a) & static_cast<uint64_t>(b));
}
