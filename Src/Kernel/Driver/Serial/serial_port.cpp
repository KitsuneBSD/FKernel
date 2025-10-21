#include <Kernel/Driver/SerialPort/serial_port.h>

void serial::init() {
  // TODO: Consider wrapping port access into a Port class with RAII and
  //       explicit volatile semantics to avoid accidental optimizations.
  // FIXME: This function uses bare port I/O and assumes the port is present
  //        and ready. Add graceful handling if COM1 is absent. Also the
  //        hardcoded baud divisor assumes a specific clock rate.
  outb(COM1 + 1, 0x00); // Desabilita interrupções
  outb(COM1 + 3, 0x80); // Ativa DLAB
  outb(COM1 + 0, 0x03); // Baud divisor low byte (38400)
  outb(COM1 + 1, 0x00); // Baud divisor high byte
  outb(COM1 + 3, 0x03); // 8 bits, sem paridade, 1 stop bit
  outb(COM1 + 2, 0xC7); // FIFO: enable, clear, 14 bytes threshold
  outb(COM1 + 4, 0x0B); // Modem: RTS/DSR
}

void serial::write_char(char c) {
  // FIXME: Busy-wait loop; consider an interrupt-driven transmit buffer or
  //        at least a timeout to avoid locking the CPU in pathological
  //        scenarios. Also this is not reentrant.
  while (!is_transmit_empty())
    ;
  outb(COM1, c);
}

void serial::write(const char *str) {
  // TODO: Provide a write(const span<char>&) overload and a bufferified
  //       path to avoid per-character overhead.
  for (size_t i = 0; str[i] != '\0'; i++) {
    write_char(str[i]);
  }
}

void serial::write_dec(int64_t value) {
  char buffer[20] = {};
  bool negative = value < 0;
  size_t i = 0;
  if (negative)
    value = -value;

  do {
    buffer[i++] = '0' + (value % 10);
    value /= 10;
  } while (value && i < sizeof(buffer));

  if (negative)
    buffer[i++] = '-';

  for (size_t j = 0; j < i / 2; j++) {
    char tmp = buffer[j];
    buffer[j] = buffer[i - j - 1];
    buffer[i - j - 1] = tmp;
  }

  write(buffer);
}

void serial::write_hex(uint64_t value) {
  char buffer[17] = {};
  const char hex_chars[] = "0123456789ABCDEF";
  for (int i = 15; i >= 0; --i) {
    buffer[i] = hex_chars[value & 0xF];
    value >>= 4;
  }
  write("0x");
  write(buffer);
}
