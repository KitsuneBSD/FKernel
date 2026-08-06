#pragma once

// Thin compatibility shim — implementation lives in LibFK.
#include <LibFK/Memory/Allocators/intrusive_free_list.h>

using SlabFreeList = fk::memory::IntrusiveFreeList;
