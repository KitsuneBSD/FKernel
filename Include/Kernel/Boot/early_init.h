#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/boot_info.h>

void early_init(multiboot2::TagMemoryMap const* mmap);