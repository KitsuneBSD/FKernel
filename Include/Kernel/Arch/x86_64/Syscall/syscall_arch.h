#pragma once

#include <LibFK/Types/types.h>

constexpr uint32_t MSR_EFER = 0xC0000080;
constexpr uint32_t MSR_STAR = 0xC0000081;
constexpr uint32_t MSR_LSTAR = 0xC0000082;
constexpr uint32_t MSR_CSTAR = 0xC0000083;
constexpr uint32_t MSR_SFMASK = 0xC0000084;
constexpr uint32_t MSR_FS_BASE = 0xC0000100;
constexpr uint32_t MSR_GS_BASE = 0xC0000101;
constexpr uint32_t MSR_KERNEL_GS_BASE = 0xC0000102;

constexpr uint64_t EFER_SCE = 1;

void init_syscalls();
