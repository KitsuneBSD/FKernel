#pragma once

#include <Kernel/Boot/multiboot2.h>

void early_init(multiboot2::TagMemoryMap const* mmap);