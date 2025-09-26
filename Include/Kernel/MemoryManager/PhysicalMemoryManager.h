#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>
#include <LibC/string.h>
#include <LibFK/bitmap.h>
#include <LibFK/rb_tree.h>

extern "C" uintptr_t __kernel_end;
extern "C" uintptr_t __heap_start;
extern "C" uintptr_t __heap_end;

struct PageRange {
  uintptr_t start;
  uintptr_t end;
  Bitmap<uint64_t, 0> *m_bitmap;
  size_t bitmap_size;

  bool operator<(const PageRange &other) const { return start < other.start; }
};

class PMM {
private:
  PMM() = default;

  rb_tree<PageRange> m_page_tree;

public:
  static PMM &the() {
    static PMM inst;
    return inst;
  }

  void initialize(const multiboot2::TagMemoryMap *mmap);
  uintptr_t allocate_page();
  void free_page(uintptr_t addr);
};
