#include <Arch/x86_64/gdt.h>
#include <LibFK/Log.h>

constexpr size_t GDT_ENTRIES = 7;

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

  // TODO: Change this implementation to a proper memset implementation using
  // SIMD/SSE2
  for (size_t i = 0; i < sizeof(TSS); ++i) {
    reinterpret_cast<uint8_t *>(&tss)[i] = 0;
  }

  // FIXME: Update rsp0 and ist1 to real values in stack
  tss.rsp0 = 0xCAFEBABE000;
  tss.ist1 = 0xDEADBEEF000;

  Logf(LogLevel::INFO, "TSS cleared and configured.");
  Logf(LogLevel::INFO, "TSS.rsp0 set to 0x%lx (kernel stack).", tss.rsp0);
  Logf(LogLevel::INFO, "TSS.ist1 set to 0x%lx (critical IST).", tss.ist1);

  // TODO: Make a right start of all stacks rsp1, rsp2 and IST 2–7
  set_tss_descriptor(reinterpret_cast<uint64_t>(&tss), sizeof(TSS) - 1);
  *(reinterpret_cast<GDT_TSS_Entry *>(&gdt[5])) = tss_descriptor;

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
