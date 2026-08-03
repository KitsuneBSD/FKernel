#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Types/types.h>

// Per-CPU IST stacks: [cpu][slot][bytes]. Each slot is a 4 KiB guard page followed by
// IST_STACK_SIZE of usable stack. GDTController::install_ist_guard_pages() unmaps the
// guard page once the VMM is up, so an IST overflow faults (kernel page fault -> panic)
// instead of silently corrupting adjacent memory.
static constexpr size_t IST_GUARD_SIZE = PAGE_SIZE;
static constexpr size_t IST_SLOT_BYTES = IST_GUARD_SIZE + IST_STACK_SIZE;
static constexpr size_t IST_STACK_OFFSET = IST_GUARD_SIZE;

alignas(PAGE_SIZE) static uint8_t ist_stacks[MAX_CPUS][7][IST_SLOT_BYTES];
