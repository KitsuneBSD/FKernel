#pragma once

#include <LibFK/Types/types.h>

namespace fkernel {
namespace smp {

extern "C" void trampoline_start();
extern "C" void trampoline_end();
extern "C" void trampoline_data();

static constexpr uintptr_t TRAMPOLINE_PHYS_ADDR = 0x8000;
static constexpr size_t   TRAMPOLINE_PAGE_COUNT = 1;

struct TrampolineData {
    uint64_t pml4_phys;
    uint64_t stack_ptr;
    uint64_t entry_point;
    uint32_t cpu_index;
    volatile uint64_t online_flag;
};

void bootstrap_aps();

} // namespace smp
} // namespace fkernel
