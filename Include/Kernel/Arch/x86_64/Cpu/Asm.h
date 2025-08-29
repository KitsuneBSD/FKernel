#pragma once

#include <LibC/stdint.h>

/*
 * === Segments Flush
 */

extern "C" void flush_gdt(void *gdtr);
extern "C" void flush_idt(void *idtr);
extern "C" void flush_tss(uint16_t selector);

/*
 * === Cr3 Control
 */
extern "C" uintptr_t read_cr3();
extern "C" void write_cr3(uintptr_t phys_addr);

/*
 * === Invalid TLB Page
 */
extern "C" void invalid_tlb(uintptr_t addr);
