#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibC/stdint.h>

extern "C" uint64_t stack_top;
extern "C" void flush_tss(uint16_t tss_selector);
extern "C" void flush_gdt(void *gdtr);

void GDTController::setupGDT() {
  setupNull();
  setupKernelCode();
  setupKernelData();
  setupTSS();
  setupGDTR();
}

void GDTController::set_kernel_stack(uint64_t stack_addr) {
  tss.rsp0 = stack_addr;
}

void GDTController::setupTSS() {
  tss.rsp0 = stack_top; // stack do kernel
  tss.ist1 = reinterpret_cast<uint64_t>(&ist_stacks[0][IST_STACK_SIZE]);
  tss.ist2 = reinterpret_cast<uint64_t>(&ist_stacks[1][IST_STACK_SIZE]);
  tss.ist3 = reinterpret_cast<uint64_t>(&ist_stacks[2][IST_STACK_SIZE]);
  tss.ist4 = reinterpret_cast<uint64_t>(&ist_stacks[3][IST_STACK_SIZE]);
  tss.ist5 = reinterpret_cast<uint64_t>(&ist_stacks[4][IST_STACK_SIZE]);
  tss.ist6 = reinterpret_cast<uint64_t>(&ist_stacks[5][IST_STACK_SIZE]);
  tss.ist7 = reinterpret_cast<uint64_t>(&ist_stacks[6][IST_STACK_SIZE]);
  tss.io_map_base = sizeof(TSS64);

  tss.rsp1 = reinterpret_cast<uint64_t>(&rsp1_stack);
  tss.rsp2 = reinterpret_cast<uint64_t>(&rsp2_stack);

  uintptr_t addr = reinterpret_cast<uintptr_t>(&tss);
  size_t size = sizeof(TSS64) - 1;

  uint64_t low = (size & 0xFFFFULL) | ((addr & 0xFFFFFFULL) << 16) |
                 (static_cast<uint64_t>(SegmentAccess::TSS64) << 40) |
                 (((size >> 16) & 0x0FULL) << 48) |
                 (((addr >> 24) & 0xFFULL) << 56);

  uint64_t high = (addr >> 32) & 0xFFFFFFFFULL;

  gdt[3] = low;
  gdt[4] = high;
  // TODO: Consider moving TSS construction into a helper class to avoid
  // raw pointer arithmetic and manual packing. Use static_asserts to
  // validate size/alignment assumptions about TSS64 and IST stacks.
  // FIXME: The code assumes IST stacks and RSP values exist at compile
  // time; add validation and fail-safe defaults for platforms where
  // the memory layout differs.
}

void GDTController::setupGDTR() {
  gdtr.limit = sizeof(gdt) - 1;
  gdtr.base = reinterpret_cast<uint64_t>(&gdt);
}

// C++
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

void GDTController::initialize() {
  uint16_t tss_selector = 0x28;

  if (m_initialized)
    return;

  setupGDT();
  setupGDTR();
  flush_gdt(&gdtr);
  loadSegments();
  klog("GDT", "Global descriptor table initialized");

  set_kernel_stack(stack_top);
  flush_tss(tss_selector);
  klog("TSS", "Task state segment initialized");
  m_initialized = true;
  // TODO: Document why tss_selector is 0x28 and centralize selector
  // generation; magic numbers for selectors are fragile across changes.
}
