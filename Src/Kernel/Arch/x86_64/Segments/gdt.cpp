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

static_assert(sizeof(TSS64) == 112, "TSS64 size unexpected; check packing/alignment");

static void setup_entries_impl(uint64_t gdt[10]) {
  gdt[0] = 0; // null

  gdt[1] = createSegment(SegmentAccess::Ring0Code,
                         SegmentFlags::LongMode | SegmentFlags::Granularity4K);
  gdt[2] = createSegment(SegmentAccess::Ring0Data, SegmentFlags::Granularity4K);
  gdt[3] = createSegment(SegmentAccess::Ring3Data, SegmentFlags::Granularity4K);
  gdt[4] = createSegment(SegmentAccess::Ring3Code,
                         SegmentFlags::LongMode | SegmentFlags::Granularity4K);
  gdt[7] = createSegment(SegmentAccess::Ring0Code,
                         SegmentFlags::DefaultSize32 | SegmentFlags::Granularity4K);
  gdt[8] = createSegment(SegmentAccess::Ring0Code, SegmentFlags::DefaultSize16);
  gdt[9] = createSegment(SegmentAccess::Ring0Data, SegmentFlags::DefaultSize16);
}

static void fill_tss_impl(TSS64& tss, uint32_t cpu_index) {
  uint64_t rsp0_top = reinterpret_cast<uint64_t>(&stack_bottom)
                      + KERNEL_STACK_SIZE * (cpu_index + 1);
  tss.rsp0 = rsp0_top;
  tss.rsp1 = 0;
  tss.rsp2 = 0;

  uint64_t *ist_targets[7] = {&tss.ist1, &tss.ist2, &tss.ist3, &tss.ist4,
                               &tss.ist5, &tss.ist6, &tss.ist7};
  for (size_t i = 0; i < 7; ++i) {
    uint64_t top = reinterpret_cast<uint64_t>(
        &ist_stacks[i][IST_STACK_SIZE]) + cpu_index * IST_STACK_SIZE * 7;
    *ist_targets[i] = top;
  }

  tss.io_map_base = sizeof(TSS64);
}

static void fill_gdt_tss_entries(uint64_t gdt[10], TSS64& tss) {
  uint64_t base = reinterpret_cast<uint64_t>(&tss);
  uint32_t limit = static_cast<uint32_t>(sizeof(TSS64) - 1);
  uint16_t limit16 = limit & 0xFFFF;
  uint64_t base0 = base & 0xFFFF;
  uint64_t base1 = (base >> 16) & 0xFF;
  uint64_t base2 = (base >> 24) & 0xFF;
  uint64_t base3 = (base >> 32) & 0xFFFFFFFF;

  uint64_t low = (limit16) | (base0 << 16) |
                 (0x9ULL << 40) | (1ULL << 47) |
                 (base1 << 32) | (base2 << 56);
  uint64_t high = base3;

  gdt[TSS_INDEX] = low;
  gdt[TSS_INDEX + 1] = high;
}

void GDTController::initialize() {
  if (m_initialized) {
    fk::algorithms::kwarn("GDT", "already initialized, skipping");
    return;
  }
  init_per_cpu(0);
  load_per_cpu(0);
  m_initialized = true;
}

void GDTController::init_per_cpu(uint32_t cpu_index) {
  auto& gdt = m_gdt_per_cpu[cpu_index];
  auto& tss = m_tss_per_cpu[cpu_index];
  auto& gdtr = m_gdtr_per_cpu[cpu_index];

  setup_entries_impl(gdt);
  fill_tss_impl(tss, cpu_index);
  fill_gdt_tss_entries(gdt, tss);

  gdtr.limit = static_cast<uint16_t>(sizeof(gdt) - 1);
  gdtr.base = reinterpret_cast<uint64_t>(&gdt);

  fk::algorithms::klog("GDT", "CPU %u GDT/TSS prepared (GDTR base=0x%016lx)",
                       cpu_index, gdtr.base);
}

void GDTController::load_per_cpu(uint32_t cpu_index) {
  auto& gdtr = m_gdtr_per_cpu[cpu_index];

  flush_gdt(&gdtr);

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

  flush_tss(TSS_SELECTOR);
}

void GDTController::set_kernel_stack(uint64_t stack_addr) {
  m_tss_per_cpu[0].rsp0 = stack_addr;
}
