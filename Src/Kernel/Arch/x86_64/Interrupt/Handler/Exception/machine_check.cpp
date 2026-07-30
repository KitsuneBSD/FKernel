#include <Kernel/Arch/x86_64/Interrupt/Handler/exception_macros.h>
#include <LibFK/Algorithms/log.h>

static void dump_mca_banks() {
  uint32_t lo, hi;

  // IA32_MCG_CAP (0x179): bits 7:0 = bank count
  asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x179u));
  uint32_t bank_count = lo & 0xFF;

  // IA32_MCG_STATUS (0x17A)
  asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x17Au));
  uint64_t mcg_status = ((uint64_t)hi << 32) | lo;
  fk::algorithms::kerror("MCA", "MCG_STATUS=%016llx banks=%u", mcg_status, bank_count);

  if (bank_count > 8) bank_count = 8; // cap for safety
  for (uint32_t i = 0; i < bank_count; ++i) {
    // IA32_MCi_STATUS = 0x401 + i*4
    asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x401u + i * 4));
    uint64_t status = ((uint64_t)hi << 32) | lo;
    if (!(status & (1ULL << 63))) continue; // MCI_STATUS.VAL not set

    uint64_t addr = 0, misc = 0;
    if (status & (1ULL << 58)) { // ADDRV
      asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x402u + i * 4));
      addr = ((uint64_t)hi << 32) | lo;
    }
    if (status & (1ULL << 59)) { // MISCV
      asm volatile("rdmsr" : "=a"(lo), "=d"(hi) : "c"(0x403u + i * 4));
      misc = ((uint64_t)hi << 32) | lo;
    }
    fk::algorithms::kerror("MCA", "Bank%u: STATUS=%016llx ADDR=%016llx MISC=%016llx",
                           i, status, addr, misc);
  }
}

void machine_check_handler(uint8_t, InterruptFrame*) {
  dump_mca_banks();
  halt_forever();
}
