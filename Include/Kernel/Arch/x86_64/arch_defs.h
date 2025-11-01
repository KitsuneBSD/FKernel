#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibC/limits.h>

#include <LibFK/Traits/types.h>

constexpr uint8_t MAX_X86_64_INTERRUPTS_LENGTH = 256;
constexpr size_t PAGE_SIZE = 4 * KiB;
constexpr uintptr_t PAGE_SIZE_2M = 2 * MiB;
constexpr size_t PAGE_MASK = (~(PAGE_SIZE - 1));
constexpr size_t MAX_CHUNKS_PER_RANGE = UINT64_MAX;
constexpr size_t IST_STACK_SIZE = 16 * KiB;
constexpr uint16_t TSS_SELECTOR = 0x28;
