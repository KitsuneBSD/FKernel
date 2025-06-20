#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

constexpr LibC::size_t GDT_ENTRIES = 7;
constexpr LibC::size_t IST_COUNT = 7;
constexpr LibC::size_t IDT_ENTRIES = 256;
constexpr LibC::size_t STACK_SIZE = 64 * 1024;

alignas(64) extern LibC::uint8_t kernel_stack[STACK_SIZE];
alignas(64) extern LibC::uint8_t ist_stacks[IST_COUNT][STACK_SIZE];
