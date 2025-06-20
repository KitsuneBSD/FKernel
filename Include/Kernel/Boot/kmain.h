#pragma once

#include <Kernel/Boot/multiboot2.h>
#include <Kernel/Boot/multiboot_interpreter.h>
#include <LibFK/types.hpp>

#ifdef __cplusplus
extern "C" {
#endif

void kmain(FK::dword multiboot_magic, void* mb_info);

#ifdef __cplusplus
}
#endif
