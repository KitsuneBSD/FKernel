#include <Kernel/Arch/x86_64/Segments/gdt.h>
#include <LibC/stdint.h>

extern "C" uint64_t stack_top;
extern "C" void flush_gdt(void *gdtr);
extern "C" void flush_tss(uint16_t tss_selector);

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
  uintptr_t addr = reinterpret_cast<uintptr_t>(&tss);
  size_t size = sizeof(TSS64) - 1;

  uint64_t low = (size & 0xFFFFULL) | ((addr & 0xFFFFFFULL) << 16) |
                 (static_cast<uint64_t>(SegmentAccess::TSS64) << 40) |
                 (((size >> 16) & 0x0FULL) << 48) |
                 (((addr >> 24) & 0xFFULL) << 56);

  uint64_t high = (addr >> 32) & 0xFFFFFFFFULL;

  gdt[3] = low;
  gdt[4] = high;
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

  set_kernel_stack(stack_top);
  flush_tss(tss_selector);
  klog("GDT", "Global descriptor table initialized");
  m_initialized = true;
}
