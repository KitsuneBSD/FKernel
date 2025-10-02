#pragma once

#include <Kernel/Arch/x86_64/io.h>
#include <LibC/stdint.h>
#include <LibFK/Algorithms/log.h>

static constexpr uint8_t PIC1_CMD = 0x20;
static constexpr uint8_t PIC1_DATA = 0x21;
static constexpr uint8_t PIC2_CMD = 0xA0;
static constexpr uint8_t PIC2_DATA = 0xA1;

static constexpr uint8_t ICW1_INIT = 0x11;
static constexpr uint8_t ICW1_ICW4 = 0x01;

static constexpr uint8_t ICW4_8086 = 0x01;

static constexpr uint8_t PIC_READ_IRR = 0x0A;
static constexpr uint8_t PIC_READ_ISR = 0x0B;

static uint16_t __get_irq_reg(uint8_t ocw3) {
  outb(PIC1_CMD, ocw3);
  outb(PIC2_CMD, ocw3);
  return (inb(PIC2_CMD) << 8) | inb(PIC1_CMD);
}

class PIC8259 {
private:
  static uint16_t get_irr() { return __get_irq_reg(PIC_READ_IRR); }
  static uint16_t get_isr() { return __get_irq_reg(PIC_READ_ISR); }

public:
  static void initialize();
  static void mask_irq(uint8_t irq);
  static void unmask_irq(uint8_t irq);
  static void send_eoi(uint8_t irq);
  static void send_eoi_safe(uint8_t irq);
};
