#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibC/stdint.h>

extern "C" uint64_t stack_top;
extern "C" uint64_t stack_bottom;
extern "C" void flush_tss(uint16_t tss_selector);
extern "C" void flush_gdt(void *gdtr);

void GDTController::setupNull() { gdt[0] = 0; }

void GDTController::setupKernelCode() {
  gdt[1] = createSegment(SegmentAccess::Ring0Code,
                         SegmentFlags::LongMode | SegmentFlags::Granularity4K);
}

void GDTController::setupKernelData() {
  gdt[2] = createSegment(SegmentAccess::Ring0Data, SegmentFlags::Granularity4K);
}

void GDTController::setupGDT() {
  kdebug("GDT", "Setting up GDT entries...");
  setupNull();
  setupKernelCode();
  setupKernelData();
  setupTSS();
  setupGDTR();
  kdebug("GDT", "GDTR configured");
}

void GDTController::setupTSS() {
  kdebug("TSS", "Initializing TSS stacks and IST entries...");

  tss.rsp0 = reinterpret_cast<uint64_t>(&stack_bottom) + 4096 * 4;

  if (tss.rsp0 == 0) {
    kdebug("TSS", "Warning: RSP0 is 0");
  }

  kdebug("TSS", "RSP0 set on %p", tss.rsp0);

  for (int i = 0; i < 7; i++) {
    tss.ist1 = (i == 0)
                   ? reinterpret_cast<uint64_t>(&ist_stacks[0][IST_STACK_SIZE])
                   : tss.ist1;
    tss.ist2 = (i == 1)
                   ? reinterpret_cast<uint64_t>(&ist_stacks[1][IST_STACK_SIZE])
                   : tss.ist2;
    tss.ist3 = (i == 2)
                   ? reinterpret_cast<uint64_t>(&ist_stacks[2][IST_STACK_SIZE])
                   : tss.ist3;
    tss.ist4 = (i == 3)
                   ? reinterpret_cast<uint64_t>(&ist_stacks[3][IST_STACK_SIZE])
                   : tss.ist4;
    tss.ist5 = (i == 4)
                   ? reinterpret_cast<uint64_t>(&ist_stacks[4][IST_STACK_SIZE])
                   : tss.ist5;
    tss.ist6 = (i == 5)
                   ? reinterpret_cast<uint64_t>(&ist_stacks[5][IST_STACK_SIZE])
                   : tss.ist6;
    tss.ist7 = (i == 6)
                   ? reinterpret_cast<uint64_t>(&ist_stacks[6][IST_STACK_SIZE])
                   : tss.ist7;

    uint64_t ist_val = 0;
    switch (i) {
    case 0:
      ist_val = tss.ist1;
      break;
    case 1:
      ist_val = tss.ist2;
      break;
    case 2:
      ist_val = tss.ist3;
      break;
    case 3:
      ist_val = tss.ist4;
      break;
    case 4:
      ist_val = tss.ist5;
      break;
    case 5:
      ist_val = tss.ist6;
      break;
    case 6:
      ist_val = tss.ist7;
      break;
    }

    if (ist_val == 0) {
      kdebug("TSS", "Warning: IST stack is 0", i);
    }

    kdebug("TSS", "IST stack %d configured %p", i, ist_val);
  }

  tss.rsp1 = reinterpret_cast<uint64_t>(&rsp1_stack);
  tss.rsp2 = reinterpret_cast<uint64_t>(&rsp2_stack);
  kdebug("TSS", "RSP1 set on %p", tss.rsp1);
  kdebug("TSS", "RSP2 set on %p", tss.rsp2);

  tss.io_map_base = sizeof(TSS64);
  kdebug("TSS", "IO map base set on %p", tss.io_map_base);

  uintptr_t addr = reinterpret_cast<uintptr_t>(&tss);
  size_t size = sizeof(TSS64) - 1;

  uint64_t low = (size & 0xFFFFULL) | ((addr & 0xFFFFFFULL) << 16) |
                 (static_cast<uint64_t>(SegmentAccess::TSS64) << 40) |
                 (((size >> 16) & 0x0FULL) << 48) |
                 (((addr >> 24) & 0xFFULL) << 56);

  uint64_t high = (addr >> 32) & 0xFFFFFFFFULL;

  gdt[3] = low;
  gdt[4] = high;

  kdebug("TSS", "TSS descriptor added to GDT", low, high);
}

void GDTController::setupGDTR() {
  gdtr.limit = sizeof(gdt) - 1;
  gdtr.base = reinterpret_cast<uint64_t>(&gdt);
}

void GDTController::loadSegments() {
  kdebug("GDT", "Loading segment registers...");
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
  kdebug("GDT", "Segment registers loaded");
}

void GDTController::initialize() {

  if (m_initialized) {
    kdebug("GDT", "GDT already initialized, skipping");
    return;
  }

  kdebug("GDT", "Starting GDT initialization...");
  setupGDT();
  setupGDTR();
  flush_gdt(&gdtr);
  loadSegments();
  kdebug("GDT", "Global descriptor table initialized");

  flush_tss(TSS_SELECTOR);
  kdebug("TSS", "Task state segment initialized");

  m_initialized = true;
  klog("GDT", "Initialization complete, TSS selector: 0x%lu", TSS_SELECTOR);
}
