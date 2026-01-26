#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <Kernel/Memory/memory_manager.h>
#include <LibC/assert.h>
#include <LibC/string.h>
#include <LibFK/Memory/heap_malloc.h>
#include <LibFK/Algorithms/log.h>

static uint32_t g_log_targets = fk::algorithms::LogTarget::Display | fk::algorithms::LogTarget::DebugFS | fk::algorithms::LogTarget::Serial;

void fk::algorithms::set_log_targets(uint32_t targets) {
    g_log_targets = targets;
}

uint32_t fk::algorithms::get_log_targets() {
    return g_log_targets;
}

extern "C" void libc_puts(char *c) {
  ASSERT(c != NULL);

  if (g_log_targets & fk::algorithms::LogTarget::Serial) {
    serial::write(c);
  }
  
  if (g_log_targets & fk::algorithms::LogTarget::Display) {
    vga::the().write_ansi(c);
  }
  
  // Also append to our internal debug log buffer if heap is ready
  if (g_log_targets & fk::algorithms::LogTarget::DebugFS) {
    if (MemoryManager::the().is_heap_initialized()) {
        auto log_node = fkernel::DebugLogNode::the();
        if (log_node) {
            log_node->append(c, strlen(c));
        }
    }
  }
}
