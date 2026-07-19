#include <LibC/stdio.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Synchronization/spinlock.h>

static uint32_t g_log_targets =
    fk::algorithms::LogTarget::Display |
    fk::algorithms::LogTarget::DebugFS |
    fk::algorithms::LogTarget::Serial;
static fk::synchronization::Spinlock g_log_lock;
static void (*g_puts_hook)(const char *) = nullptr;
static bool g_heap_ready = false;

extern "C" void libc_set_heap_ready()   { g_heap_ready = true; }
extern "C" bool libc_is_heap_ready()    { return g_heap_ready; }

extern "C" void libc_register_puts_hook(void (*fn)(const char *)) {
    g_puts_hook = fn;
}

void fk::algorithms::set_log_targets(uint32_t targets) {
    g_log_targets = targets;
}

uint32_t fk::algorithms::get_log_targets() {
    return g_log_targets;
}

extern "C" void libc_puts(char *c) {
    if (!c) return;
    fk::synchronization::ScopedLockIRQ lock(g_log_lock);
    if (g_puts_hook) g_puts_hook(c);
}
