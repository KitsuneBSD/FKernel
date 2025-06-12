#pragma once

#include <Boot/multiboot2.h>
#include <Boot/multiboot_interpreter.h>
#include <Driver/Vga_Buffer.hpp>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void kmain(uint32_t multiboot_magic, void *mb_info);

#ifdef __cplusplus
}
#endif
