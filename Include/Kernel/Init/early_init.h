#pragma once

#include <Kernel/Arch/x86_64/gdt.h>
#include <Kernel/Arch/x86_64/idt.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

void early_init(uint64_t memory_available);
