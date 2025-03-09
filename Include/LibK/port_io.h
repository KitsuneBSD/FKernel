#pragma once

#include "stdbool.h"
#include "stdint.h"

#define fence() __asm__ volatile("" ::: "memory")

static inline void wrmsr(uint64_t msr, uint64_t value) {
  uint32_t low = value & 0xFFFFFFFF;
  uint32_t high = value >> 32;
  asm volatile("wrmsr" : : "c"(msr), "a"(low), "d"(high));
}

static inline uint64_t rdmsr(uint64_t msr) {
  uint32_t low;
  uint32_t high;
  asm volatile("rdmsr" : "=a"(low), "=d"(high) : "c"(msr));
  return ((uint64_t)high << 32) | low;
}

static inline void invlpg(void *m) {
  asm volatile("invlpg (%0)" : : "b"(m) : "memory");
}

static inline void outb(uint16_t port, uint8_t val) {
  __asm__ volatile("outb %b0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void outw(uint16_t port, uint16_t val) {
  __asm__ volatile("outw %w0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void outl(uint16_t port, uint32_t val) {
  __asm__ volatile("outl %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline void outq(uint16_t port, uint64_t val) {
  __asm__ volatile("outq %0, %w1" : : "a"(val), "Nd"(port) : "memory");
}

static inline uint8_t inb(uint16_t port) {
  uint8_t ret;
  __asm__ volatile("inb %w1, %b0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline uint16_t inw(uint16_t port) {
  uint16_t ret;
  __asm__ volatile("inw %w1, %w0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline uint32_t inl(uint16_t port) {
  uint32_t ret;
  __asm__ volatile("inl %w1, %0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline uint64_t inq(uint16_t port) {
  uint64_t ret;
  __asm__ volatile("inq %w1, %0" : "=a"(ret) : "Nd"(port) : "memory");
  return ret;
}

static inline unsigned long read_cr0(void) {
  unsigned long val;
  asm volatile("mov %%cr0, %0" : "=r"(val));
  return val;
}

static inline unsigned long read_cr2(void) {
  unsigned long val;
  asm volatile("mov %%cr2, %0" : "=r"(val));
  return val;
}

static inline unsigned long read_cr3(void) {
  unsigned long val;
  asm volatile("mov %%cr3, %0" : "=r"(val));
  return val;
}

static inline uint64_t rdtsc() {
  uint64_t ret;
  asm volatile("rdtsc" : "=A"(ret));
  return ret;
}

static inline void cpuid(int code, const uint32_t *a, const uint32_t *d) {
  asm volatile("cpuid" : "=a"(a), "=d"(d) : "0"(code) : "ebx", "ecx");
}

static inline void lidt(void *base, uint16_t size) {
  struct {
    uint16_t length;
    void *base;
  } __attribute__((packed)) IDTR = {size, base};

  asm volatile("lidt %0" : : "m"(IDTR));
}

static inline bool are_interrupts_enabled() {
  unsigned long flags;
  asm volatile("pushf\n\t"
               "pop %0"
               : "=g"(flags));
  return flags & (1 << 9);
}

static inline unsigned long save_irqdisable(void) {
  unsigned long flags;
  asm volatile("pushf\n\tcli\n\tpop %0" : "=r"(flags) : : "memory");
  return flags;
}

static inline void irqrestore(unsigned long flags) {
  asm volatile("push %0\n\tpopf" : : "rm"(flags) : "memory", "cc");
}

static inline void io_wait(void) { outb(0x80, 0); }
