/**
 * @file zone_defs.h
 * @brief Constants and definitions for physical memory zones.
 */

#pragma once 

#include <LibFK/Types/types.h>

/// @brief Size of the bitmap used for page tracking.
static constexpr uint64_t BITMAP_SIZE = 1 * fk::types::MiB;

/// @brief Physical address limit for DMA-capable memory (16MB).
static constexpr uintptr_t DMA_LIMIT  = 16ull * 1024 * 1024;

/// @brief Physical address limit for "Normal" memory (4GB).
static constexpr uintptr_t HIGH_LIMIT = 4ull  * 1024 * 1024 * 1024;

/// @brief Standard page frame size (4KB).
static constexpr size_t FRAME_SIZE = 4096;

