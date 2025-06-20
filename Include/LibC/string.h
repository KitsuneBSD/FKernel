#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace LibC {

LibC::size_t utoa(LibC::uint64_t value, char* buffer, int base);

void* memcpy(void* dest, void const* src, LibC::size_t n);

void* memset(void* dest, int ch, LibC::size_t n);

int atoi(char const* str);

char* itoa(int num, char* str, int base);

LibC::size_t strlen(char const* str);
}
