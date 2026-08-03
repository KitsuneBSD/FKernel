#pragma once

#include <LibFK/Types/types.h>

struct Termios {
  uint32_t c_iflag;
  uint32_t c_oflag;
  uint32_t c_cflag;
  uint32_t c_lflag;
  uint8_t  c_cc[32];

  static constexpr uint32_t ICANON  = 0x00000002;
  static constexpr uint32_t ECHO    = 0x00000008;
  static constexpr uint32_t ECHOE   = 0x00000010;
  static constexpr uint32_t ISIG    = 0x00000001;
  static constexpr uint32_t ECHONL  = 0x00000040;

  static constexpr uint8_t VINTR   = 0;
  static constexpr uint8_t VQUIT   = 1;
  static constexpr uint8_t VERASE  = 2;
  static constexpr uint8_t VKILL   = 3;
  static constexpr uint8_t VEOF    = 4;
  static constexpr uint8_t VEOL    = 5;
  static constexpr uint8_t VSUSP   = 10;
  static constexpr uint8_t VSTART  = 11;
  static constexpr uint8_t VSTOP   = 12;
  static constexpr uint8_t VWERASE = 14;
  static constexpr uint8_t VLNEXT  = 15;

  void set_defaults() {
    c_iflag = 0x2502;
    c_oflag = 0x0005;
    c_cflag = 0x04cb;
    c_lflag = ICANON | ECHO | ECHOE | ISIG | ECHONL;
    for (size_t i = 0; i < 32; ++i) c_cc[i] = 0;
    c_cc[VINTR]   = 0x03;
    c_cc[VQUIT]   = 0x1C;
    c_cc[VERASE]  = 0x7F;
    c_cc[VKILL]   = 0x15;
    c_cc[VEOF]    = 0x04;
    c_cc[VSUSP]   = 0x1A;
    c_cc[VWERASE] = 0x17;
    c_cc[VLNEXT]  = 0x16;
  }

  bool has_lflag(uint32_t flag) const { return (c_lflag & flag) != 0; }
  bool has_oflag(uint32_t flag) const { return (c_oflag & flag) != 0; }

  static constexpr uint32_t OPOST = 0x00000001;
  static constexpr uint32_t ONLCR = 0x00000004;
};
