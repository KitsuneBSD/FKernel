#pragma once 

#include <Kernel/Memory/PhysicalMemory/Buddy/BuddyAllocator.h>
#include <Kernel/Memory/PhysicalMemory/Zone/ZoneAllocator.h>
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
  Zone zone;
  BuddyAllocator buddy;
  fk::containers::Bitmap<uint64_t> bitmap;
  bool is_initialized{false};
};
