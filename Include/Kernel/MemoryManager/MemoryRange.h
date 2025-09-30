#pragma once 

#include <LibC/stdint.h>
#include <LibC/stddef.h>

enum class MemoryType : uint32_t {
  Usable,
  Reserved,
};

struct PhysicalMemoryRange {
  uintptr_t m_start;
  uintptr_t m_end;
  MemoryType m_type;

  bool operator<(const PhysicalMemoryRange &other) const {
    return m_start < other.m_start;
  }

  bool operator==(const PhysicalMemoryRange &other) const {
    bool is_same_start = m_start == other.m_start;
    bool is_same_end = m_end == other.m_end;
    bool is_same_type = m_type == other.m_type;

    return is_same_start && is_same_end && is_same_type;
  }

  bool contains(uintptr_t addr) const {
    return addr >= m_start && addr < m_end;
  }

  uintptr_t size() const { return m_end - m_start; }
};