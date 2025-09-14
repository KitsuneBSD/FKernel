#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/Io.h>
#endif // __x86_64__

class Serial {
private:
  static constexpr uint16_t COM1 = 0x3F8;

  static inline bool is_transmit_empty() { return inb(COM1 + 5) & 0x20; }

public:
  static void init() {
    outb(COM1 + 1, 0x00); // Desabilita interrupções
    outb(COM1 + 3, 0x80); // Ativa DLAB
    outb(COM1 + 0, 0x03); // Baud divisor low byte (38400)
    outb(COM1 + 1, 0x00); // Baud divisor high byte
    outb(COM1 + 3, 0x03); // 8 bits, sem paridade, 1 stop bit
    outb(COM1 + 2, 0xC7); // FIFO: enable, clear, 14 bytes threshold
    outb(COM1 + 4, 0x0B); // Modem: RTS/DSR
  }

  static void write_char(char c) {
    while (!is_transmit_empty())
      ;
    outb(COM1, c);
  }

  static void write(const char *str) {
    for (size_t i = 0; str[i] != '\0'; i++) {
      write_char(str[i]);
    }
  }

  static void write_dec(int64_t value) {
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

  static void write_hex(uint64_t value) {
    char buffer[17] = {};
    static const char hex_chars[] = "0123456789ABCDEF";
    for (int i = 15; i >= 0; --i) {
      buffer[i] = hex_chars[value & 0xF];
      value >>= 4;
    }
    write("0x");
    write(buffer);
  }
};
