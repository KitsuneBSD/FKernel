#pragma once

#include <LibC/stdint.h>

extern "C" void flush_gdt(void* gdtr);
extern "C" void flush_idt(void* idtr);
extern "C" void flush_tss(LibC::uint16_t selector);
