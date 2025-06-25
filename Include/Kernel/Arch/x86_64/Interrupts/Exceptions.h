#pragma once

#include <Kernel/Arch/x86_64/Cpu/State.h>
#include <LibC/stdint.h>

extern void (*exception_stubs[32])();

void global_default_handler(struct CpuState const* frame);
char const* named_exception(int index) noexcept;
