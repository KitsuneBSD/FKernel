#pragma once

#include <Arch/x86_64/io.h>
#include <LibC/stdint.h>

constexpr uint16_t PIC1_CMD = 0x20;
constexpr uint16_t PIC1_DATA = 0x21;
constexpr uint16_t PIC2_CMD = 0xA0;
constexpr uint16_t PIC2_DATA = 0xA1;

constexpr uint8_t ICW1_INIT = 0x10;
constexpr uint8_t ICW1_ICW4 = 0x01;

constexpr uint8_t ICW4_8086 = 0x01;

void remap(int offset1, int offset2);
void send_eoi(uint8_t irq);
void set_mask(uint8_t mask);
uint8_t get_mask();
void disable_pic();
