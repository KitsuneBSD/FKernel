#include <Kernel/Hardware/Cpu.h>
#include <LibFK/Algorithms/log.h>

void CPU::write_msr(uint32_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
  kdebug("CPU", "Wrote MSR 0x%08x = 0x%016lx", msr, value);
}

uint64_t CPU::read_msr(uint32_t msr) {
  uint32_t low, high;
  asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  uint64_t value = ((uint64_t)high << 32) | low;
  kdebug("CPU", "Read MSR 0x%08x = 0x%016lx", msr, value);
  return value;
}
