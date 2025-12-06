#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/Segments/gdt_structures.h>
#include <Kernel/Arch/x86_64/Segments/tss_stacks.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

extern "C" uint64_t stack_top;
extern "C" uint64_t stack_bottom;
extern "C" void flush_tss(uint16_t tss_selector);
extern "C" void flush_gdt(void *gdtr);

static constexpr size_t EXPECTED_TSS_SIZE = sizeof(TSS64);
static_assert(EXPECTED_TSS_SIZE == 112,
              "TSS64 size unexpected; check structure packing/alignment");

void GDTController::setupTSS() {
  fk::algorithms::kdebug("TSS",
                         "Initializing TSS structure and IST entries...");

  uint64_t rsp0_top =
      reinterpret_cast<uint64_t>(&stack_bottom) + KERNEL_STACK_SIZE;
  tss.rsp0 = rsp0_top;
  fk::algorithms::kdebug("TSS", "RSP0 set to 0x%016lx", tss.rsp0);

  tss.rsp1 = reinterpret_cast<uint64_t>(&rsp1_stack[IST_STACK_SIZE]);
  tss.rsp2 = reinterpret_cast<uint64_t>(&rsp2_stack[IST_STACK_SIZE]);
  fk::algorithms::kdebug("TSS", "RSP1 = 0x%016lx, RSP2 = 0x%016lx", tss.rsp1,
                         tss.rsp2);

  uint64_t *ist_targets[7] = {&tss.ist1, &tss.ist2, &tss.ist3, &tss.ist4,
                              &tss.ist5, &tss.ist6, &tss.ist7};

  for (size_t i = 0; i < 7; ++i) {
    uint64_t top = reinterpret_cast<uint64_t>(&ist_stacks[i][IST_STACK_SIZE]);
    *ist_targets[i] = top;
    fk::algorithms::kdebug("TSS", "IST[%zu] top = 0x%016lx (stack=%p size=%u)",
                           i + 1, *ist_targets[i], &ist_stacks[i],
                           IST_STACK_SIZE);
  }

  tss.io_map_base = sizeof(TSS64);
  fk::algorithms::kdebug("TSS", "I/O map base = %u (TSS size = %u)",
                         tss.io_map_base, sizeof(TSS64));

  uint64_t base = reinterpret_cast<uint64_t>(&tss);
  uint32_t limit = static_cast<uint32_t>(sizeof(TSS64) - 1);

  fk::algorithms::kdebug("TSS", "TSS: %p (base=0x%016lx) limit=0x%x", &tss,
                         base, limit);

  uint16_t limit16 = limit & 0xFFFF;

  uint64_t base0 = base & 0xFFFF;
  uint64_t base1 = (base >> 16) & 0xFF;
  uint64_t base2 = (base >> 24) & 0xFF;
  uint64_t base3 = (base >> 32) & 0xFFFFFFFF;

  uint64_t low = (limit16) | (base0 << 16) |
                 ((uint64_t)0x9 << 40) | // type = 9 (available TSS)
                 ((uint64_t)1 << 47) |   // present
                 (base1 << 32) | (base2 << 56);

  uint64_t high = base3;

  if (TSS_INDEX + 1 >= (sizeof(gdt) / sizeof(gdt[0]))) {
    fk::algorithms::kerror("TSS", "TSS_INDEX out of range");
  }

  gdt[TSS_INDEX] = low;
  gdt[TSS_INDEX + 1] = high;

  if (gdt[TSS_INDEX] != low || gdt[TSS_INDEX + 1] != high) {
    fk::algorithms::kwarn("TSS", "GDT write/readback mismatch");
    fk::algorithms::kdebug("TSS", "Expected LOW  = 0x%016lx", low);
    fk::algorithms::kdebug("TSS", "Actual   LOW  = 0x%016lx", gdt[TSS_INDEX]);
    fk::algorithms::kdebug("TSS", "Expected HIGH = 0x%016lx", high);
    fk::algorithms::kdebug("TSS", "Actual   HIGH = 0x%016lx",
                           gdt[TSS_INDEX + 1]);
    fk::algorithms::kerror("GDT", "Write/Readback failed for TSS descriptor");
  }

  fk::algorithms::kdebug("TSS", "TSS Descriptor LOW  = 0x%016lx", low);
  fk::algorithms::kdebug("TSS", "TSS Descriptor HIGH = 0x%016lx", high);
  fk::algorithms::kdebug("TSS", "TSS selector expected = 0x%04x (index=%zu)",
                         TSS_SELECTOR, TSS_INDEX);
}

void GDTController::setupGDTR() {
  gdtr.limit = static_cast<uint16_t>(sizeof(gdt) - 1);
  gdtr.base = reinterpret_cast<uint64_t>(&gdt);

  // Checagens básicas de alinhamento/limite
  if ((gdtr.base & 0x7ULL) != 0) {
    fk::algorithms::kwarn("GDT", "GDTR base (0x%016lx) not 8-byte aligned",
                          gdtr.base);
  }
  fk::algorithms::kdebug("GDT", "GDTR prepared: base=0x%016lx limit=0x%04x",
                         gdtr.base, gdtr.limit);
}

void GDTController::loadSegments() {
  fk::algorithms::kdebug(
      "GDT", "Reloading segment registers (CS=0x08, DS=ES=FS=GS=SS=0x10)...");
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
  fk::algorithms::kdebug("GDT", "Segment registers successfully reloaded");
}

void GDTController::setupGDT() {
  fk::algorithms::kdebug("GDT", "Initializing GDT entries...");
  setupNull();
  setupKernelCode();
  setupKernelData();
  setupUserCode();
  setupUserData();
  setupTSS();
  setupGDTR();
  fk::algorithms::kdebug("GDT", "GDT setup completed (entries=%zu)",
                         sizeof(gdt) / sizeof(gdt[0]));
}

void GDTController::initialize() {
  if (m_initialized) {
    fk::algorithms::kdebug("GDT", "GDT already initialized, skipping");
    return;
  }

  fk::algorithms::kdebug("GDT",
                         "Starting GDT and TSS initialization sequence...");
  setupGDT();

  flush_gdt(&gdtr);
  fk::algorithms::kdebug("GDT", "flush_gdt() called - lgdt expected");

  GDTR loaded_gdtr = {};
  asm volatile("sgdt %0" : "=m"(loaded_gdtr));
  fk::algorithms::kdebug("GDT", "SGDT returned: base=0x%016lx limit=0x%04x",
                         loaded_gdtr.base, loaded_gdtr.limit);
  if (loaded_gdtr.base != gdtr.base || loaded_gdtr.limit != gdtr.limit) {
    fk::algorithms::kerror("GDT", "GDTR mismatch after lgdt");
  }

  loadSegments();
  fk::algorithms::kdebug("GDT", "Segment registers and selectors are live");

  fk::algorithms::kdebug("TSS", "Loading TSS with selector 0x%04x",
                         TSS_SELECTOR);
  flush_tss(TSS_SELECTOR);

  uint16_t tr_val = 0;
  asm volatile("str %0" : "=r"(tr_val));
  fk::algorithms::kdebug("TSS", "STR returned 0x%04x", tr_val);
  if ((tr_val & 0xFFF8) != (TSS_SELECTOR & 0xFFF8)) {
    fk::algorithms::kwarn(
        "TSS", "Loaded TR (0x%04x) does not match expected selector (0x%04x)",
        tr_val, TSS_SELECTOR);
    fk::algorithms::kerror("TSS", "Load verification failed (STR mismatch)");
  }

  m_initialized = true;
  fk::algorithms::klog(
      "GDT",
      "Initialization complete (TSS selector=0x%04x, GDTR base=0x%016lx)",
      TSS_SELECTOR, gdtr.base);
}
