#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#ifdef __x86_64__
#include <Kernel/Arch/x86_64/io.h>
#endif // __x86_64__

static constexpr uint16_t COM1 = 0x3F8;

class serial {
private:
  static inline bool is_transmit_empty() { return inb(COM1 + 5) & 0x20; }

public:
  static void init();
  static void write_char(char c);

  static void write(const char *str);
  static void write_dec(int64_t value);
  static void write_hex(uint64_t value);
};
