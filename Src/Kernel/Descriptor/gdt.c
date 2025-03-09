#include "../../../Include/Kernel/Descriptor/gdt.h"
#include "../../../Include/Kernel/Driver/vga_buffer.h"

gdt_entry_t gdt_entry[5];
gdt_ptr_t gdt_ptr;

void create_descriptor(uint32_t base, uint32_t limit, uint16_t flag) {
  int index = (flag >> 5) & 0x03;

  gdt_entry[index].limit_low = (limit & 0xFFFF);
  gdt_entry[index].base_low = (base & 0xFFFF);
  gdt_entry[index].base_middle = (base >> 16) & 0xFF;
  gdt_entry[index].access = (flag & 0xFF);
  gdt_entry[index].granularity = ((limit >> 16) & 0x0F) | (flag >> 8);
  gdt_entry[index].base_high = (base >> 24) & 0xFF;
}

void init_gdt() {
  print_str("Load Null Descriptor\n");
  create_descriptor(0, 0, 0);
  print_str("Load the Kernel Segments\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_0);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_0);
  print_str("Load the Driver Segments\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_1);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_1);
  print_str("Load the Virtualization Segments\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_2);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_2);
  print_str("Load the User Segments\n");
  create_descriptor(0, 0xFFFFFFFF, GDT_CODE_RING_3);
  create_descriptor(0, 0xFFFFFFFF, GDT_DATA_RING_3);

  gdt_ptr.limit = (sizeof(gdt_entry) - 1);
  gdt_ptr.base = (uint64_t)&gdt_entry;
}
