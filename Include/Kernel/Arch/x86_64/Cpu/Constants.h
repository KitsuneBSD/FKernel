#pragma once

#include <LibC/stdint.h>

constexpr LibC::uint8_t IST_COUNT = 7;
constexpr LibC::uint16_t TSS_SELECTOR = 5 << 3;

constexpr LibC::uint8_t IDT_TYPE_INTERRUPT_GATE = 0x8E; // P=1, DPL=00, Type=1110
constexpr LibC::uint8_t IDT_TYPE_TRAP_GATE = 0x8F;      // P=1, DPL=00, Type=1111
