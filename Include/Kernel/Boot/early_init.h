#pragma once

#include <Kernel/Arch/x86_64/gdt.h>
#include <Kernel/Boot/multiboot2.h>
#include <LibC/stdint.h>

void early_init(multiboot2::TagMemoryMap);
