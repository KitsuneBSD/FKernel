#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/Log.h>

void bump_allocator_init(uint64_t start, uint64_t size);

void *bump_alloc_aligned(size_t size, size_t alignment);

void *bump_alloc(size_t size);

void *bump_allocator_current();

void bump_allocator_reset();
