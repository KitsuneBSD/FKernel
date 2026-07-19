#include <Kernel/Io/kernel_puts.h>
#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <Kernel/Fs/DebugFs/debug_fs.h>
#include <LibC/stdio.h>
#include <LibC/string.h>
#include <LibFK/Algorithms/log.h>

static void kernel_puts_impl(const char *c) {
    uint32_t targets = fk::algorithms::get_log_targets();
    if (targets & fk::algorithms::LogTarget::Serial)
        serial::write(c);
    if (targets & fk::algorithms::LogTarget::Display)
        vga::the().write_ansi(c);
    if (targets & fk::algorithms::LogTarget::DebugFS) {
        auto node = fkernel::DebugLogNode::the();
        if (node) node->append(c, strlen(c));
    }
}

namespace fkernel {
namespace io {

void initialize_kernel_puts() {
    libc_register_puts_hook(kernel_puts_impl);
}

} // namespace io
} // namespace fkernel
