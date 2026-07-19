#include <tests/test_framework.h>

void __kernel_assert_fail(const char *expr, const char *file, int line, const char *func) {
    TEST_LOG("ASSERTION FAILED: %s\n", expr);
    TEST_LOG("at %s:%d in %s\n", file, line, func);
    exit(1);
}

/* Heap allocator mocks — map to host libc */
void* kmalloc(size_t size) { return malloc(size); }
void  kfree(void* ptr)     { free(ptr); }
void* kcalloc(size_t n, size_t s) { return calloc(n, s); }
void* krealloc(void* p, size_t s) { return realloc(p, s); }
void* heap_malloc(size_t size) { return malloc(size); }
void  heap_free(void* ptr)     { free(ptr); }

/* LibC kernel I/O mocks */
void kprintf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    vprintf(fmt, args);
    va_end(args);
}

void libc_puts(const char *s) { puts(s); }
void libc_putc(char c) { putchar((unsigned char)c); }
void libc_set_heap_ready(void) { /* no-op in test */ }
