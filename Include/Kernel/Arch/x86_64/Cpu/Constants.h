#pragma once

#include "LibC/stddef.h"
#include <LibC/stdint.h>
#include <LibFK/types.h>

constexpr LibC::uint8_t IST_COUNT = 7;
constexpr LibC::uint16_t TSS_SELECTOR = 5 << 3;

constexpr LibC::uint8_t IDT_TYPE_INTERRUPT_GATE = 0x8E; // P=1, DPL=00, Type=1110
constexpr LibC::uint8_t IDT_TYPE_TRAP_GATE = 0x8F;      // P=1, DPL=00, Type=1111

constexpr LibC::uint64_t TOTAL_MEMORY_PAGE_SIZE = 4096;

constexpr LibC::size_t ENTRIES_PER_PAGE = 512;

static constexpr LibC::uint8_t PIC1_CMD = 0x20;
static constexpr LibC::uint8_t PIC1_DATA = 0x21;
static constexpr LibC::uint8_t PIC2_CMD = 0xA0;
static constexpr LibC::uint8_t PIC2_DATA = 0xA1;

static constexpr LibC::uint8_t ICW1_INIT = 0x10;
static constexpr LibC::uint8_t ICW1_ICW4 = 0x01;
static constexpr LibC::uint8_t ICW4_8086 = 0x01;

static LibC::uint64_t max_region_in_bytes = 64 * static_cast<LibC::uint64_t>(FK::MiB);
static LibC::uint64_t max_region_in_pages = max_region_in_bytes / TOTAL_MEMORY_PAGE_SIZE;
