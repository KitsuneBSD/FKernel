#pragma once

#include <Kernel/Memory/ObjectMemory/slab_free_list.h>
#include <LibFK/Types/types.h>

struct SlabCache;

struct Slab {
  SlabCache *cache;
  Slab *next;
  SlabFreeList free_list;
};
