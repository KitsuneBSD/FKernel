#pragma once

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

void kmain(uint32_t multiboot_magic, void *mb_info);

#ifdef __cplusplus
}
#endif
