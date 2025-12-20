#include <Kernel/Driver/SerialPort/serial_port.h>
#include <Kernel/Driver/Vga/vga_adapter.h>
#include <LibC/assert.h>

extern "C" void libc_puts(char *c) {
  ASSERT(c != NULL);
  serial::write(c);
  vga::the().write_ansi(c);
}
