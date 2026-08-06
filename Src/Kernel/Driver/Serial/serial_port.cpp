#include <Kernel/Driver/Serial/serial_port.h>

void Serial::init() {
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x80);
  outb(COM1 + 0, 0x03);
  outb(COM1 + 1, 0x00);
  outb(COM1 + 3, 0x03);
  outb(COM1 + 2, 0xC7);
  outb(COM1 + 4, 0x0B);
}

void Serial::write_char(char c) {
  while (!is_transmit_empty())
    ;
  outb(COM1, c);
}

void Serial::write_buffer(const char *data, size_t len) {
  static constexpr size_t FIFO_DEPTH = 16;
  size_t i = 0;
  while (i < len) {
    while (!is_transmit_empty())
      ;
    size_t chunk = len - i;
    if (chunk > FIFO_DEPTH) chunk = FIFO_DEPTH;
    for (size_t j = 0; j < chunk; ++j)
      outb(COM1, data[i + j]);
    i += chunk;
  }
}

void Serial::write(const char *str) {
  size_t len = 0;
  while (str[len]) ++len;
  write_buffer(str, len);
}

void Serial::write_dec(int64_t value) {
  char buffer[20] = {};
  bool negative = value < 0;
  size_t i = 0;
  uint64_t uvalue = negative ? (~static_cast<uint64_t>(value) + 1u) : static_cast<uint64_t>(value);

  do {
    buffer[i++] = '0' + (uvalue % 10);
    uvalue /= 10;
  } while (uvalue && i < sizeof(buffer));

  if (negative)
    buffer[i++] = '-';

  for (size_t j = 0; j < i / 2; j++) {
    char tmp = buffer[j];
    buffer[j] = buffer[i - j - 1];
    buffer[i - j - 1] = tmp;
  }

  write(buffer);
}

size_t Serial::read(uint8_t* buf, size_t max) {
  size_t n = 0;
  while (n < max && is_data_ready())
    buf[n++] = static_cast<uint8_t>(inb(COM1));
  return n;
}

void Serial::write_hex(uint64_t value) {
  char buffer[17] = {};
  const char hex_chars[] = "0123456789ABCDEF";
  for (int i = 15; i >= 0; --i) {
    buffer[i] = hex_chars[value & 0xF];
    value >>= 4;
  }
  write("0x");
  write(buffer);
}
