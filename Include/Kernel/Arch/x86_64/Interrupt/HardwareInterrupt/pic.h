#pragma once

#include <LibC/stdint.h>

static constexpr uint8_t PIC1_CMD = 0x20;
static constexpr uint8_t PIC1_DATA = 0x21;
static constexpr uint8_t PIC2_CMD = 0xA0;
static constexpr uint8_t PIC2_DATA = 0xA1;

static constexpr uint8_t EOI = 0x20;

class Pic8259 {
public:
  static void remap(uint8_t offset1 = 0x20, uint8_t offset2 = 0x28);
  static void sendEOI(uint8_t irq);
  static void set_mask(uint8_t irq);
  static void clear_mask(uint8_t irq);
  static uint8_t get_mask_master();
  static uint8_t get_mask_slave();
  static void set_mask_master(uint8_t mask);
  static void set_mask_slave(uint8_t mask);
};
