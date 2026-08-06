#include <LibFK/Algorithms/Logging/log.h>
#include <Kernel/Arch/x86_64/Hardware/Cpu/cpu_ops.h>
#include <Kernel/Hardware/Cpu/cpu.h>
#include <Kernel/Hardware/Cpu/mtrr.h>

namespace fkernel::mtrr {

static MtrrState s_bsp{};
static bool      s_saved = false;

void save_bsp() {
    uint64_t cap        = CPU::the().read_msr(MSR_MTRRCAP);
    s_bsp.var_count     = static_cast<uint8_t>(cap & 0xFF);
    s_bsp.has_fixed     = (cap >> 8) & 1;
    s_bsp.def_type      = CPU::the().read_msr(MSR_MTRR_DEF_TYPE);

    if (s_bsp.has_fixed) {
        s_bsp.fix64k    = CPU::the().read_msr(MSR_MTRR_FIX64K);
        s_bsp.fix16k[0] = CPU::the().read_msr(MSR_MTRR_FIX16K_80);
        s_bsp.fix16k[1] = CPU::the().read_msr(MSR_MTRR_FIX16K_A0);
        for (uint8_t i = 0; i < 8; ++i)
            s_bsp.fix4k[i] = CPU::the().read_msr(MSR_MTRR_FIX4K_BASE + i);
    }

    uint8_t n = s_bsp.var_count < MAX_VAR_MTRR ? s_bsp.var_count : MAX_VAR_MTRR;
    for (uint8_t i = 0; i < n; ++i) {
        s_bsp.var_base[i] = CPU::the().read_msr(MSR_MTRR_PHYS_BASE + i * 2);
        s_bsp.var_mask[i] = CPU::the().read_msr(MSR_MTRR_PHYS_BASE + i * 2 + 1);
    }

    s_saved = true;
    fk::algorithms::klog("MTRR", "BSP MTRR saved: %u var ranges, fixed=%d", n, (int)s_bsp.has_fixed);
}

void load_on_ap() {
    if (!s_saved) return;

    // Intel SDM Vol 3A §11.11.8: disable caching, disable MTRRs, write, re-enable.
    uint64_t cr0 = arch_read_cr0();
    arch_write_cr0(cr0 | (1ULL << 30));     // set CD
    arch_wbinvd();
    arch_flush_tlb();

    CPU::the().write_msr(MSR_MTRR_DEF_TYPE, s_bsp.def_type & ~(1ULL << 11));

    if (s_bsp.has_fixed) {
        CPU::the().write_msr(MSR_MTRR_FIX64K,    s_bsp.fix64k);
        CPU::the().write_msr(MSR_MTRR_FIX16K_80, s_bsp.fix16k[0]);
        CPU::the().write_msr(MSR_MTRR_FIX16K_A0, s_bsp.fix16k[1]);
        for (uint8_t i = 0; i < 8; ++i)
            CPU::the().write_msr(MSR_MTRR_FIX4K_BASE + i, s_bsp.fix4k[i]);
    }

    uint8_t n = s_bsp.var_count < MAX_VAR_MTRR ? s_bsp.var_count : MAX_VAR_MTRR;
    for (uint8_t i = 0; i < n; ++i) {
        CPU::the().write_msr(MSR_MTRR_PHYS_BASE + i * 2,     s_bsp.var_base[i]);
        CPU::the().write_msr(MSR_MTRR_PHYS_BASE + i * 2 + 1, s_bsp.var_mask[i]);
    }

    CPU::the().write_msr(MSR_MTRR_DEF_TYPE, s_bsp.def_type);

    arch_wbinvd();
    arch_flush_tlb();
    arch_write_cr0(cr0);
}

} // namespace fkernel::mtrr
