#pragma once

#include <LibFK/Types/types.h>

class InodeIndex {
  uint64_t m_index;

public:
  explicit InodeIndex(uint64_t index) : m_index(index) {}
  uint64_t value() const { return m_index; }
  bool operator==(const InodeIndex &other) const {
    return m_index == other.m_index;
  }
};
