#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>
#include <LibFK/types.h>

#include <Kernel/Arch/x86_64/Cpu/Gdt_Constants.h>
#include <Kernel/Arch/x86_64/Hardware/Io_Constants.h>

constexpr LibC::uint8_t IST_COUNT = 7;
constexpr LibC::uint16_t TSS_SELECTOR = 5 << 3;

constexpr LibC::uint8_t IDT_TYPE_INTERRUPT_GATE = 0x8E; // P=1, DPL=00, Type=1110
constexpr LibC::uint8_t IDT_TYPE_TRAP_GATE = 0x8F;      // P=1, DPL=00, Type=1111

constexpr LibC::uint64_t TOTAL_MEMORY_PAGE_SIZE = 4096;

constexpr LibC::uint64_t PAGE_ADDRESS_MASK = 0x000FFFFFFFFFF000ULL;
constexpr LibC::size_t ENTRIES_PER_PAGE = 512;

constexpr LibC::uint8_t PIC1_CMD = 0x20;
constexpr LibC::uint8_t PIC1_DATA = 0x21;
constexpr LibC::uint8_t PIC2_CMD = 0xA0;
constexpr LibC::uint8_t PIC2_DATA = 0xA1;

constexpr LibC::uintptr_t KERNEL_IDENTITY_LIMIT = 0x40000000; // 1 GiB
constexpr LibC::uintptr_t KERNEL_PHYS_BASE = 0xFFFF800000000000;

constexpr LibC::uint32_t PIT_FREQUENCY = 1193182;

constexpr LibC::uint16_t PIT_CHANNEL0_PORT = 0x40;
constexpr LibC::uint16_t PIT_CHANNEL1_PORT = 0x41;
constexpr LibC::uint16_t PIT_CHANNEL2_PORT = 0x42;
constexpr LibC::uint16_t PIT_COMMAND_PORT = 0x43;

constexpr LibC::uint8_t PIT_COMMAND = 0x36;

constexpr LibC::uint64_t ADDR_MASK = 0x000FFFFFFFFFF000ULL;

constexpr LibC::uint64_t MASK_BITS = 0x1FFULL;

constexpr LibC::uint64_t PAGE_MASK = 0xFFFFFFFFFFFFF000ULL;

static LibC::uint64_t max_region_in_bytes = 64 * static_cast<LibC::uint64_t>(FK::MiB);
static LibC::uint64_t max_region_in_pages = max_region_in_bytes / TOTAL_MEMORY_PAGE_SIZE;
