#pragma once 

#include <Kernel/Memory/PhysicalMemory/Buddy/buddy_allocator.h>
#include <Kernel/Memory/ObjectMemory/Zone/zone_allocator.h>
#include <LibFK/Container/bitmap.h>

/**
 * @brief Represents a physical memory zone in the system.
 * 
 * A PhysicalZone encapsulates a contiguous region of physical memory and provides
 * hierarchical memory management. It combines three layers:
 * 
 * - Zone: Manages the overall memory region and its type (DMA, NORMAL, HIGH).
 * - BuddyAllocator: Handles large block allocations within the zone efficiently.
 * - Bitmap: Tracks individual page allocations (4KB per page) within the buddy blocks.
 * 
 * This structure allows fast allocation of pages and contiguous blocks while supporting
 * a flexible hierarchical memory model. Each PhysicalZone can manage multiple buddies
 * and bitmaps internally.
 */
struct PhysicalZone {
  Zone zone;           ///< Metadata about the physical range and type.
  BuddyAllocator buddy; ///< Allocator for large contiguous blocks.
  fk::containers::Bitmap<uint64_t> bitmap; ///< Tracker for individual 4KB pages.
  bool is_initialized{false}; ///< Initialization status.
};
