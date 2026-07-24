#include <LibC/stdio.h>

static void (*g_puts_hook)(const char *) = nullptr;
static bool g_heap_ready = false;

extern "C" void libc_set_heap_ready()   { g_heap_ready = true; }
extern "C" bool libc_is_heap_ready()    { return g_heap_ready; }

extern "C" void libc_register_puts_hook(void (*fn)(const char *)) {
    g_puts_hook = fn;
}

extern "C" void libc_puts(char *c) {
    if (!c) return;
    if (g_puts_hook) g_puts_hook(c);
}
