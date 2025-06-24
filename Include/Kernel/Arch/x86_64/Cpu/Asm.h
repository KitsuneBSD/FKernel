#pragma once

extern "C" void flush_gdt(void* gdtr);
extern "C" void flush_idt(void* idtr);
