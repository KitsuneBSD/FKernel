#pragma once

#include <LibFK/Types/types.h>

namespace fkernel::mtrr {

constexpr uint32_t MSR_MTRRCAP         = 0x00FE;
constexpr uint32_t MSR_MTRR_DEF_TYPE   = 0x02FF;
constexpr uint32_t MSR_MTRR_FIX64K     = 0x0250;
constexpr uint32_t MSR_MTRR_FIX16K_80  = 0x0258;
constexpr uint32_t MSR_MTRR_FIX16K_A0  = 0x0259;
constexpr uint32_t MSR_MTRR_FIX4K_BASE = 0x0268;
constexpr uint32_t MSR_MTRR_PHYS_BASE  = 0x0200;
constexpr uint8_t  MAX_VAR_MTRR        = 10;

struct MtrrState {
    uint64_t def_type;
    uint64_t fix64k;
    uint64_t fix16k[2];
    uint64_t fix4k[8];
    uint64_t var_base[MAX_VAR_MTRR];
    uint64_t var_mask[MAX_VAR_MTRR];
    uint8_t  var_count;
    bool     has_fixed;
};

// Called once on the BSP before start_aps(); stores values in a file-static.
void save_bsp();

// Called on each AP during ap_entry to mirror BSP MTRR configuration.
void load_on_ap();

} // namespace fkernel::mtrr
