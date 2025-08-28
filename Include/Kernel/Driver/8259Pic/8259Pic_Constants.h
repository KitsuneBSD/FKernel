#pragma once

#include <LibC/stdint.h>

static constexpr LibC::uint8_t ICW1_INIT = 0x10;
static constexpr LibC::uint8_t ICW1_ICW4 = 0x01;
static constexpr LibC::uint8_t ICW4_8086 = 0x01;
static constexpr LibC::uint8_t PIC_EOI = 0x20;
