#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Types/types.h>

/**
 * @brief Statically allocated Interrupt Stack Table (IST) stacks
 *
 * Aligns stacks to 16 bytes for x86_64 ABI compliance.
 */
alignas(16) static uint8_t ist_stacks[7][IST_STACK_SIZE];
alignas(16) static uint8_t rsp1_stack[IST_STACK_SIZE];
alignas(16) static uint8_t rsp2_stack[IST_STACK_SIZE];
