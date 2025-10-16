#pragma once

#include <LibC/stdint.h>

static inline void io_wait() { asm volatile("outb %%al, $0x80" : : "a"(0)); }

static inline void outb(uint16_t port, uint8_t value)
{
  asm volatile("outb %0, %1" : : "a"(value), "Nd"(port));
}

static inline uint8_t inb(uint16_t port)
{
  uint8_t ret;
  asm volatile("inb %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline uint16_t inw(uint16_t port)
{
  uint16_t ret;
  asm volatile("inw %1, %0" : "=a"(ret) : "Nd"(port));
  return ret;
}

static inline void outw(uint16_t port, uint16_t value)
{
  asm volatile("outw %0, %1" : : "a"(value), "Nd"(port));
}