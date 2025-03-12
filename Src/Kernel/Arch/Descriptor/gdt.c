#include "../../../../Include/Kernel/Arch/Descriptor/gdt.h"
#include "../../../../Include/Kernel/Driver/vga_buffer.h"

struct gdt_entry gdt[MAX_GDT_ENTRIES];
struct gdt_ptr gdtr;

static void gdt_set_gate(int num, uint32_t base, uint32_t limit, uint8_t access,
                         uint8_t granularity) {
  gdt[num].base_low = (base & 0xFFFF);
  gdt[num].base_middle = (base >> 16) & 0xFF;
  gdt[num].base_high = (base >> 24) & 0xFF;

  gdt[num].limit_low = (limit & 0xFFFF);
  gdt[num].granularity = ((limit >> 16) & 0x0F);

  gdt[num].granularity |= (granularity & 0xF0);
  gdt[num].access = access;
}

void init_gdt() {
  print_str("Init GDT\n");
  gdtr.limit = (sizeof(struct gdt_entry) * MAX_GDT_ENTRIES) - 1;
  gdtr.base = (uint64_t)&gdt;

  print_str("Set NULL Descriptor\n");
  gdt_set_gate(0, 0, 0, 0, 0);

  print_str("Set Ring 0: Kernel\n");
  gdt_set_gate(1, 0, 0xFFFFF, 0x9A, 0x20);
  gdt_set_gate(2, 0, 0xFFFFF, 0x92, 0x00);

  print_str("Set Ring 3: User\n");
  gdt_set_gate(3, 0, 0xFFFFF, 0xFA, 0x20);
  gdt_set_gate(4, 0, 0xFFFFF, 0xF2, 0x00);

  gdt_flush((uint64_t)&gdtr);

  print_str("GDT Loaded\n");
}
