#include <LibC/string.h>
#include <LibFK/Algorithms/Logging/log.h>
#include <LibFK/Core/assertions.h>
#include <LibFK/Memory/Allocators/allocator_backend.h>
#include <LibFK/Memory/Allocators/heap_malloc.h>

namespace fk {
namespace memory {

static const AllocatorBackend *s_backend = nullptr;

void set_allocator_backend(const AllocatorBackend *backend) {
  s_backend = backend;
}

const AllocatorBackend *get_allocator_backend() { return s_backend; }

static inline void *backend_allocate(size_t size) {
  if (s_backend && s_backend->allocate)
    return s_backend->allocate(size);
  return nullptr;
}

static inline void *backend_reallocate(void *ptr, size_t size) {
  if (s_backend && s_backend->reallocate)
    return s_backend->reallocate(ptr, size);
  return nullptr;
}

static inline void backend_free(void *ptr) {
  if (s_backend && s_backend->free)
    s_backend->free(ptr);
}

fk::core::Result<uint64_t *, fk::core::Error> allocate(size_t size) {
  void *ptr = backend_allocate(size);
  if (!ptr)
    return fk::core::Error::OutOfMemory;
  return reinterpret_cast<uint64_t *>(ptr);
}

fk::core::Result<uint64_t *, fk::core::Error> reallocate(uint64_t *ptr,
                                                          size_t size) {
  void *new_ptr = backend_reallocate(ptr, size);
  if (!new_ptr && size > 0)
    return fk::core::Error::OutOfMemory;
  return reinterpret_cast<uint64_t *>(new_ptr);
}

void free(uint64_t *ptr) { backend_free(ptr); }

} // namespace memory
} // namespace fk

extern "C" {
void *heap_malloc(size_t size) { return fk::memory::backend_allocate(size); }
void heap_free(void *ptr) { fk::memory::backend_free(ptr); }
void *kmalloc(size_t size) { return fk::memory::backend_allocate(size); }
void kfree(void *ptr) { fk::memory::backend_free(ptr); }

void *kcalloc(size_t nmemb, size_t size) {
  if (nmemb && size > (size_t)-1 / nmemb)
    return nullptr;
  size_t total = nmemb * size;
  void *ptr = kmalloc(total);
  if (ptr)
    memset(ptr, 0, total);
  return ptr;
}

void *krealloc(void *ptr, size_t size) {
  return fk::memory::backend_reallocate(ptr, size);
}
}
