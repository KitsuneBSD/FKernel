#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

constexpr size_t GDT_ENTRIES = 7;
constexpr size_t IST_COUNT = 7;
constexpr size_t STACK_SIZE = 64 * 1024;

alignas(16) static uint8_t kernel_stack[STACK_SIZE];
alignas(16) static uint8_t ist_stacks[IST_COUNT][STACK_SIZE];
