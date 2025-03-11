#include "../../../../Include/Kernel/Arch/Descriptor/gdt.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

#define GDT_MAX_ENTRIES 8

extern void load_gdt(uint64_t gdt_ptr_address);

gdt_entry_t gdt_entry[GDT_MAX_ENTRIES];
gdt_ptr_t gdt_ptr;

int gdt_index = 0;

void create_descriptor(uint32_t base, uint32_t limit, uint16_t flag) {
  if (gdt_index > GDT_MAX_ENTRIES) {
    print_str("GDT Index reach the max entries");
    return;
  }

  gdt_entry[gdt_index].limit_low = (limit & 0xFFFF);
  gdt_entry[gdt_index].base_low = (base & 0xFFFF);
  gdt_entry[gdt_index].base_middle = (base >> 16) & 0xFF;
  gdt_entry[gdt_index].access = (flag & 0xFF);
  gdt_entry[gdt_index].granularity = ((limit >> 16) & 0x0F) | (flag >> 8);
  gdt_entry[gdt_index].base_high = (base >> 24) & 0xFF;
  ++gdt_index;
}

void init_gdt() {
  print_str("Init GDT\n");
  print_str("Load NULL Descriptor\n");
  create_descriptor(0, 0, 0);
  print_str("Load Kernel Segment\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_0);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_0);
  print_str("Load Driver Segment\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_1);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_1);
  print_str("Load Virtualization Segment\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_2);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_2);
  print_str("Load User Segment\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_3);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_3);

  gdt_ptr.limit = (sizeof(gdt_entry) - 1);
  gdt_ptr.base = (uint64_t)&gdt_entry;

  load_gdt((uint64_t)&gdt_ptr);
  print_str("GDT Initialized\n");
}
