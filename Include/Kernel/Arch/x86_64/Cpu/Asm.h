#pragma once

#include <LibC/stdint.h>

/*
 * === Segments Flush
 */

extern "C" void flush_gdt(void* gdtr);
extern "C" void flush_idt(void* idtr);
extern "C" void flush_tss(LibC::uint16_t selector);

/*
 * === Invalid TLB Page
 */
extern "C" void invalid_tlb(LibC::uintptr_t addr);
