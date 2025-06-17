#pragma once

#include <LibC/stdint.h>
#include <LibFK/Log.h>

enum class AllocatorType : uint8_t { Bump, TLSF };

void init_memory_management(uint64_t heap_start, uint64_t heap_size);
void switch_allocator(AllocatorType type);

void *kmalloc(size_t size);
void kfree(void *ptr);
