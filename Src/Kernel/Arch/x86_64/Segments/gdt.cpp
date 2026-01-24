#include <Kernel/Arch/x86_64/Segments/Gdt/gdt_structures.h>
#include <Kernel/Arch/x86_64/Segments/Tss/tss_stacks.h>
#include <Kernel/Arch/x86_64/Segments/gdt.h>
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

void GDTController::setupNull() { gdt[0] = 0; }

void GDTController::setupKernelCode() {
  gdt[1] = createSegment(SegmentAccess::Ring0Code,
                         SegmentFlags::LongMode | SegmentFlags::Granularity4K);
}

void GDTController::setupKernelData() {
  gdt[2] = createSegment(SegmentAccess::Ring0Data, SegmentFlags::Granularity4K);
}

void GDTController::setupUserCode() {
  gdt[4] = createSegment(SegmentAccess::Ring3Code,
                         SegmentFlags::LongMode | SegmentFlags::Granularity4K);
}

void GDTController::setupUserData() {
  gdt[3] = createSegment(SegmentAccess::Ring3Data, SegmentFlags::Granularity4K);
}

void GDTController::setupTSS() {

  uint64_t rsp0_top =
      reinterpret_cast<uint64_t>(&stack_bottom) + KERNEL_STACK_SIZE;
  tss.rsp0 = rsp0_top;

  tss.rsp1 = reinterpret_cast<uint64_t>(&rsp1_stack[IST_STACK_SIZE]);
  tss.rsp2 = reinterpret_cast<uint64_t>(&rsp2_stack[IST_STACK_SIZE]);

  uint64_t *ist_targets[7] = {&tss.ist1, &tss.ist2, &tss.ist3, &tss.ist4,
                              &tss.ist5, &tss.ist6, &tss.ist7};

  for (size_t i = 0; i < 7; ++i) {
    uint64_t top = reinterpret_cast<uint64_t>(&ist_stacks[i][IST_STACK_SIZE]);
    *ist_targets[i] = top;
  }

  tss.io_map_base = sizeof(TSS64);

  uint64_t base = reinterpret_cast<uint64_t>(&tss);
  uint32_t limit = static_cast<uint32_t>(sizeof(TSS64) - 1);

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
}

void GDTController::setupGDTR() {
  gdtr.limit = static_cast<uint16_t>(sizeof(gdt) - 1);
  gdtr.base = reinterpret_cast<uint64_t>(&gdt);

  if ((gdtr.base & 0x7ULL) != 0) {
    fk::algorithms::kwarn("GDT", "GDTR base (0x%016lx) not 8-byte aligned",
                          gdtr.base);
  }
}

void GDTController::loadSegments() {

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
}

void GDTController::setupGDT() {

  setupNull();
  setupKernelCode();
  setupKernelData();
  setupUserCode();
  setupUserData();
  setupTSS();
  setupGDTR();
}

void GDTController::initialize() {
  if (m_initialized) {
    fk::algorithms::kwarn("GDT", "GDT already initialized, skipping");
    return;
  }

  setupGDT();

  flush_gdt(&gdtr);

  GDTR loaded_gdtr = {};
  asm volatile("sgdt %0" : "=m"(loaded_gdtr));

  if (loaded_gdtr.base != gdtr.base || loaded_gdtr.limit != gdtr.limit) {
    fk::algorithms::kerror("GDT", "GDTR mismatch after lgdt");
  }

  loadSegments();

  flush_tss(TSS_SELECTOR);

  uint16_t tr_val = 0;
  asm volatile("str %0" : "=r"(tr_val));

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

void GDTController::set_kernel_stack(uint64_t stack_addr) {
  tss.rsp0 = stack_addr;
}
