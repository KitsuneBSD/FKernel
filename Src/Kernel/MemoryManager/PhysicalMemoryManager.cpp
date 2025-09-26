#include "LibFK/rb_tree.h"
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <LibC/limits.h>
#include <LibFK/log.h>

inline PageRange create_page_range(uintptr_t start, uintptr_t end) {
  start = (start + PAGE_SIZE - 1) & ~(PAGE_SIZE - 1);
  end &= ~(PAGE_SIZE - 1);

  if (start >= end)
    return PageRange{};

  PageRange range{};
  range.start = start;
  range.end = end;

  size_t pages = (end - start) / PAGE_SIZE;
  range.bitmap_size = pages;

  size_t bitmap_elems = (pages + 63) / 64;
  range.m_bitmap = new Bitmap<uint64_t, 0>[bitmap_elems]();

  for (size_t i = 0; i < bitmap_elems; ++i)
    range.m_bitmap[i].clear();

  return range;
}

// FIXME: This code has a bug on, when the system has more than 2GB dispare
// general-protection-fault
//
// NOTE: Separe in multi-step probably is the best step to fix this
void PMM::initialize(const multiboot2::TagMemoryMap *mmap) {
  klog("PMM", "Initialize the physical memory manager");

  size_t ranges_created = 0;
  size_t total_pages = 0;

  for (auto entry = mmap->begin(); entry != mmap->end(); ++entry) {
    if (!multiboot2::is_available(entry->type))
      continue;

    uintptr_t range_start = entry->base_addr;
    uintptr_t range_end = entry->base_addr + entry->length;

    if (range_end <= reinterpret_cast<uintptr_t>(&__kernel_end))
      continue;
    if (range_start < reinterpret_cast<uintptr_t>(&__kernel_end))
      range_start = reinterpret_cast<uintptr_t>(&__kernel_end);

    if (range_end <= reinterpret_cast<uintptr_t>(&__heap_end))
      continue;
    if (range_start < reinterpret_cast<uintptr_t>(&__heap_end))
      range_start = reinterpret_cast<uintptr_t>(&__heap_end);

    if (range_start >= range_end) {
      kerror("PMM", "Invalid range: %p - %p\n", range_start, range_end);
      continue;
    }

    PageRange pr = create_page_range(range_start, range_end);
    m_page_tree.insert(new rb_node<PageRange>(pr));
    ranges_created++;
    total_pages += pr.bitmap_size;

    klog("PMM", "PageRange created: %p - %p (%lu pages)", range_start,
         range_end, pr.bitmap_size);
  }

  uint64_t total_bytes = static_cast<uint64_t>(total_pages) * PAGE_SIZE;
  uint64_t gb = total_bytes / (1024ULL * 1024ULL * 1024ULL);
  uint64_t remainder_bytes = total_bytes % (1024ULL * 1024ULL * 1024ULL);
  uint64_t mb = remainder_bytes / (1024ULL * 1024ULL);

  if (ranges_created == 0)
    kerror("PMM", "No one PageRange valid founded!\n");
  else
    klog("PMM",
         "Physical memory manager initialized with %lu pages (~%lu GB %lu MB",
         total_pages, gb, mb);
}
