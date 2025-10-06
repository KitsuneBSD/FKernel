#pragma once

#include <Kernel/MemoryManager/TlsfHeap.h>

#pragma once
#include <LibC/stddef.h>
#include <LibC/stdint.h>

void *malloc(size_t size);
void *calloc(size_t nmemb, size_t size);
void *realloc(void *ptr, size_t size);
void free(void *ptr);