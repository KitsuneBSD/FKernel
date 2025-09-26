#include <Kernel/Arch/x86_64/arch_defs.h>
#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <Kernel/MemoryManager/PhysicalMemoryManager.h>
#include <LibC/limits.h>
#include <LibFK/log.h>

void PMM::initialize(const multiboot2::TagMemoryMap *mmap) {
  kprintf("[PMM] Scanning memory map...\n");

  PageRange *last_range = nullptr;

  for (auto *entry = mmap->entries;
       (uintptr_t)entry < (uintptr_t)mmap + mmap->size;
       entry = (multiboot2::TagMemoryMap::Entry *)((uintptr_t)entry +
                                                   mmap->entry_size)) {

    if (!multiboot2::is_available(entry->type))
      continue;

    kprintf("[PMM] Memory entry: base=0x%lx, length=0x%lx\n", entry->base_addr,
            entry->length);

    auto *range = reinterpret_cast<PageRange *>(__kernel_end);
    range->start = entry->base_addr;
    range->end = entry->base_addr + entry->length;
    size_t total_pages = (range->end - range->start) / 4096;
    range->bitmap_size = (total_pages + 63) / 64;
    range->bitmap =
        reinterpret_cast<uint64_t *>(__kernel_end + sizeof(PageRange));
    range->next = nullptr;

    for (size_t i = 0; i < range->bitmap_size; i++)
      range->bitmap[i] = UINT64_MAX;

    for (size_t i = 0; i < total_pages; i++)
      clear_bit(range, i);

    if (!head)
      head = range;
    else
      last_range->next = range;

    last_range = range;

    kprintf("[PMM] Added range: 0x%lx - 0x%lx, total pages: %lu\n",
            range->start, range->end, total_pages);
  }

  kprintf("[PMM] Initialization complete.\n");
}

uintptr_t PMM::allocate_page() {
  for (auto *range = head; range; range = range->next) {
    size_t total_pages = (range->end - range->start) / 4096;
    for (size_t i = 0; i < total_pages; i++) {
      if (!test_bit(range, i)) {
        set_bit(range, i);
        return range->start + i * 4096;
      }
    }
  }
  return 0;
}

void PMM::free_page(uintptr_t addr) {
  for (auto *range = head; range; range = range->next) {
    if (addr >= range->start && addr < range->end) {
      size_t index = (addr - range->start) / 4096;
      clear_bit(range, index);
      return;
    }
  }
}
