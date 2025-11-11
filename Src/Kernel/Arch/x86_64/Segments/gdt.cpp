#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/Segments/gdt_structures.h>
#include <Kernel/Arch/x86_64/Segments/tss_stacks.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Types/types.h>

extern "C" uint64_t stack_top;
extern "C" uint64_t stack_bottom;
extern "C" void flush_tss(uint16_t tss_selector);
extern "C" void flush_gdt(void *gdtr);

void GDTController::setupTSS() {
  kdebug("TSS", "Initializing TSS and IST entries...");

  constexpr size_t KERNEL_STACK_SIZE = 4096 * 4;

  // Configuração da pilha de ring 0
  tss.rsp0 = reinterpret_cast<uint64_t>(&stack_bottom) + KERNEL_STACK_SIZE;
  kdebug("TSS", "Ring 0 stack (RSP0) set to %p", tss.rsp0);

  // Configuração das pilhas IST
  uint64_t *ist_targets[7] = {&tss.ist1, &tss.ist2, &tss.ist3, &tss.ist4,
                              &tss.ist5, &tss.ist6, &tss.ist7};

  for (size_t i = 0; i < 7; ++i) {
    *ist_targets[i] =
        reinterpret_cast<uint64_t>(&ist_stacks[i][IST_STACK_SIZE - 1]);
    kdebug("TSS", "IST[%zu] configured at %p", i + 1, *ist_targets[i]);
  }

  tss.rsp1 = reinterpret_cast<uint64_t>(&rsp1_stack[IST_STACK_SIZE - 1]);
  tss.rsp2 = reinterpret_cast<uint64_t>(&rsp2_stack[IST_STACK_SIZE - 1]);
  kdebug("TSS", "RSP1 set to %p", tss.rsp1);
  kdebug("TSS", "RSP2 set to %p", tss.rsp2);

  tss.io_map_base = sizeof(TSS64);
  kdebug("TSS", "IO map base offset set to %u bytes", tss.io_map_base);

  uintptr_t base = reinterpret_cast<uintptr_t>(&tss);
  uint16_t limit = static_cast<uint16_t>(sizeof(TSS64) - 1);

  uint64_t low = ((limit & 0xFFFFULL)) | ((base & 0xFFFFFFULL) << 16) |
                 (static_cast<uint64_t>(SegmentAccess::TSS64Type) << 40) |
                 (((limit >> 16) & 0x0FULL) << 48) |
                 ((base & 0xFF000000ULL) << 32);

  uint64_t high = (base >> 32) & 0xFFFFFFFFULL;

  gdt[5] = low;
  gdt[6] = high;

  kdebug("TSS", "TSS descriptor created: low=%lx high=%lx", low, high);
}

void GDTController::setupGDTR() {
  gdtr.limit = static_cast<uint16_t>(sizeof(gdt) - 1);
  gdtr.base = reinterpret_cast<uint64_t>(&gdt);

  const uint16_t gdt_size = gdtr.limit + 1;
  const size_t entry_count = gdt_size / sizeof(uint64_t);

  kdebug("GDT",
         "GDTR configured:\n"
         "  Base address = %p\n"
         "  Limit        = %u (raw)\n"
         "  GDT size     = %u bytes\n"
         "  Entries      = %zu",
         gdtr.base, gdtr.limit, gdt_size, entry_count);
}

void GDTController::loadSegments() {
  kdebug("GDT",
         "Reloading segment registers (CS=0x08, DS=ES=FS=GS=SS=0x10)...");
  asm volatile("mov $0x10, %%ax\n"
               "mov %%ax, %%ds\n"
               "mov %%ax, %%es\n"
               "mov %%ax, %%fs\n"
               "mov %%ax, %%gs\n"
               "mov %%ax, %%ss\n"
               "pushq $0x08\n"
               "lea 1f(%%rip), %%rax\n"
               "push %%rax\n"
               "lretq\n"
               "1:\n"
               :
               :
               : "rax");
  kdebug("GDT", "Segment registers successfully reloaded");
}

void GDTController::setupGDT() {
  kdebug("GDT", "Initializing GDT entries...");
  setupNull();
  setupKernelCode();
  setupKernelData();
  setupUserCode();
  setupUserData();
  setupTSS();
  setupGDTR();
  kdebug("GDT", "GDT setup completed (total entries: %zu)", sizeof(gdt) / 8);
}

void GDTController::initialize() {
  if (m_initialized) {
    kdebug("GDT", "GDT already initialized, skipping");
    return;
  }

  kdebug("GDT", "Starting GDT and TSS initialization sequence...");
  setupGDT();
  flush_gdt(&gdtr);
  kdebug("GDT", "GDT loaded into GDTR via lgdt");

  loadSegments();
  kdebug("GDT", "Segment registers and selectors are live");

  flush_tss(TSS_SELECTOR);
  kdebug("TSS", "TSS loaded via ltr (selector=%lx)", TSS_SELECTOR);

  m_initialized = true;
  klog("GDT", "Initialization complete (TSS selector=%lx, base=%p)",
       TSS_SELECTOR, gdtr.base);
}
