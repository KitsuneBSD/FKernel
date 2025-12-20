#pragma once 

#include <LibFK/Types/types.h>

static constexpr uint64_t BITMAP_SIZE = 1 * fk::types::MiB;
static constexpr uintptr_t DMA_LIMIT  = 16ull * 1024 * 1024;
static constexpr uintptr_t HIGH_LIMIT = 4ull  * 1024 * 1024 * 1024;
static constexpr size_t FRAME_SIZE = 4096;

