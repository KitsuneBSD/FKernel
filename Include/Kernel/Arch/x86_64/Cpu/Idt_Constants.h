#pragma once

#include <LibC/stdint.h>

namespace idt {
constexpr int MAX_IDT_ENTRIES = 256;
constexpr int MAX_IRQ = 16;
constexpr uint16_t KERNEL_CODE_SELECTOR = 0x08;
constexpr uint8_t IDT_TYPE_INTERRUPT_GATE = 0x8E;
constexpr uint8_t IST_MASK = 0x07;

constexpr uint8_t PIC1_COMMAND = 0x20;
constexpr uint8_t PIC2_COMMAND = 0xA0;
constexpr uint8_t PIC_EOI = 0x20;

constexpr int IRQ_VECTOR_BASE = 0x20;
constexpr int IRQ_SLAVE_OFFSET = 8;
} // namespace idt
