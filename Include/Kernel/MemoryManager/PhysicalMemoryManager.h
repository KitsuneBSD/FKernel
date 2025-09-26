#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>
#include <LibC/string.h>

extern "C" uintptr_t __kernel_end;
extern "C" uintptr_t __heap_start;
extern "C" uintptr_t __heap_end;

struct PageRange {
  uintptr_t start;
  uintptr_t end;
  uint64_t *bitmap;
  size_t bitmap_size;
  struct PageRange *next;
};

class PMM {
private:
  PageRange *head = nullptr;
  PMM() = default;

  inline void set_bit(PageRange *range, size_t index) {
    range->bitmap[index / 64] |= (1ULL << (index % 64));
  }
  inline void clear_bit(PageRange *range, size_t index) {
    range->bitmap[index / 64] &= ~(1ULL << (index % 64));
  }
  inline bool test_bit(PageRange *range, size_t index) {
    return range->bitmap[index / 64] & (1ULL << (index % 64));
  }

public:
  static PMM &the() {
    static PMM inst;
    return inst;
  }

  void initialize(const multiboot2::TagMemoryMap *mmap);
  uintptr_t allocate_page();
  void free_page(uintptr_t addr);
};
