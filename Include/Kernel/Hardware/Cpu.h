#pragma once

#include <LibC/stdint.h>
#include <LibC/string.h>

#include <LibFK/log.h>

class CPU {
private:
  bool h_apic = false;
  bool h_hypervisor = false;

  CPU() {
    uint64_t a, b, c, d;
    asm volatile("cpuid" : "=a"(a), "=b"(b), "=c"(c), "=d"(d) : "a"(1));
    h_apic = d & (1 << 9);
    h_hypervisor = c & (1 << 31);
  }

public:
  static CPU &the() {
    static CPU inst;
    return inst;
  }

  void write_msr(uint32_t msr, uint64_t value);
  uint64_t read_msr(uint32_t msr);

  bool has_apic() { return this->h_apic; }
  bool has_hypervisor() { return this->h_hypervisor; }
};
