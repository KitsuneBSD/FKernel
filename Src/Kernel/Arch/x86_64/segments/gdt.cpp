
#include <Arch/x86_64/gdt.h>
#include <LibC/string.h>
#include <LibFK/Log.h>

alignas(0x1000) GDTEntry gdt[GDT_ENTRIES];
GDT_TSS_Entry tss_descriptor;
TSS tss;
GDTPointer gdtp;

void set_entry(int i, uint32_t base, uint32_t limit, uint8_t access,
               uint8_t gran) {
  gdt[i].limit_low = limit & 0xFFFF;
  gdt[i].base_low = base & 0xFFFF;
  gdt[i].base_middle = (base >> 16) & 0xFF;
  gdt[i].access = access;
  gdt[i].granularity = ((limit >> 16) & 0x0F) | (gran & 0xF0);
  gdt[i].base_high = (base >> 24) & 0xFF;
}

void set_tss_descriptor(uint64_t base, uint32_t limit) {
  tss_descriptor.limit_low = limit & 0xFFFF;
  tss_descriptor.base_low = base & 0xFFFF;
  tss_descriptor.base_middle = (base >> 16) & 0xFF;
  tss_descriptor.access = 0x89; // Present, TSS (available)
  tss_descriptor.granularity = ((limit >> 16) & 0x0F);
  tss_descriptor.base_high = (base >> 24) & 0xFF;
  tss_descriptor.base_upper = (base >> 32) & 0xFFFFFFFF;
  tss_descriptor.reserved = 0;
}

void init_gdt() {
  Logf(LogLevel::INFO, "Initializing Global Descriptor Table (GDT)...");

  set_entry(0, 0, 0, 0, 0);       // Null
  set_entry(1, 0, 0, 0x9A, 0x20); // Kernel Code (64-bit)
  set_entry(2, 0, 0, 0x92, 0x00); // Kernel Data
  set_entry(3, 0, 0, 0xFA, 0x20); // User Code (Ring 3)
  set_entry(4, 0, 0, 0xF2, 0x00); // User Data (Ring 3)

  Logf(LogLevel::INFO, "Standard segments (kernel/user) initialized.");

  memset(&tss, 0, sizeof(TSS));

  tss.rsp0 = reinterpret_cast<uint64_t>(&kernel_stack[STACK_SIZE]);

  tss.ist1 = reinterpret_cast<uint64_t>(&ist_stacks[0][STACK_SIZE]);
  tss.ist2 = reinterpret_cast<uint64_t>(&ist_stacks[1][STACK_SIZE]);
  tss.ist3 = reinterpret_cast<uint64_t>(&ist_stacks[2][STACK_SIZE]);
  tss.ist4 = reinterpret_cast<uint64_t>(&ist_stacks[3][STACK_SIZE]);
  tss.ist5 = reinterpret_cast<uint64_t>(&ist_stacks[4][STACK_SIZE]);
  tss.ist6 = reinterpret_cast<uint64_t>(&ist_stacks[5][STACK_SIZE]);
  tss.ist7 = reinterpret_cast<uint64_t>(&ist_stacks[6][STACK_SIZE]);

  for (int i = 0; i < IST_COUNT; ++i) {
    Logf(LogLevel::INFO, "TSS.ist%d set to 0x%lx", i + 1,
         reinterpret_cast<uint64_t>(&ist_stacks[i][STACK_SIZE]));
  }

  Logf(LogLevel::INFO, "TSS cleared and configured.");
  Logf(LogLevel::INFO, "TSS.rsp0 set to 0x%lx (kernel stack).", tss.rsp0);

  set_tss_descriptor(reinterpret_cast<uint64_t>(&tss), sizeof(TSS) - 1);

  constexpr size_t TSS_ENTRY_INDEX = 5;
  static_assert(TSS_ENTRY_INDEX + 1 < GDT_ENTRIES, "TSS overflows GDT");

  memcpy(&gdt[TSS_ENTRY_INDEX], &tss_descriptor, sizeof(tss_descriptor));

  Logf(LogLevel::INFO, "TSS descriptor written to GDT (entries 5 and 6).");

  gdtp.limit = sizeof(GDTEntry) * 5 + sizeof(GDT_TSS_Entry) - 1;
  gdtp.base = reinterpret_cast<uint64_t>(&gdt);

  Logf(LogLevel::INFO, "GDT pointer constructed at 0x%lx with limit 0x%x.",
       gdtp.base, gdtp.limit);

  Logf(LogLevel::INFO, "Executing lgdt...");
  gdt_flush(&gdtp);
  Logf(LogLevel::INFO, "GDT successfully loaded.");

  Logf(LogLevel::INFO, "Loading TSS with selector 0x28...");
  tss_flush(0x28);
  Logf(LogLevel::INFO, "TSS successfully loaded with ltr.");
}
