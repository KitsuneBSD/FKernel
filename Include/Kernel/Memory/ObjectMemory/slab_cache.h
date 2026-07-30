#pragma once

#include <LibFK/Types/types.h>

struct Slab;

struct SlabCache {
  size_t object_size;
  size_t objects_per_slab;
  size_t slab_header_offset;
  Slab *partial_slabs;
  size_t total_objects;
  size_t total_free;
};
