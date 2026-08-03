#pragma once

#include <LibFK/Types/types.h>

namespace fk {
namespace memory {

struct AllocatorBackend {
  void *(*allocate)(size_t size);
  void *(*reallocate)(void *ptr, size_t size);
  void (*free)(void *ptr);
};

void set_allocator_backend(const AllocatorBackend *backend);
const AllocatorBackend *get_allocator_backend();

} // namespace memory
} // namespace fk
