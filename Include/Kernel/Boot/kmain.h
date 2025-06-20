#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>

#ifdef __cplusplus
extern "C" {
#endif

void kmain(LibC::uint32_t multiboot_magic, void* mb_info);

#ifdef __cplusplus
}
#endif
