#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

namespace LibC {

LibC::size_t utoa(LibC::uint64_t value, char* buffer, int base, bool uppercase = true);

extern "C" void* memcpy(void* dest, void const* src, LibC::size_t n);
extern "C" void* memset(void* dest, int ch, LibC::size_t n);

int atoi(char const* str);

char* itoa(int num, char* str, int base);

extern "C" int strcmp(char const* a, char const* b);
extern "C" int strncmp(char const* s1, char const* s2, LibC::size_t n);
extern "C" char* strncpy(char* dest, char const* src, LibC::size_t n);

extern "C" LibC::size_t strlen(char const* str);

extern "C" LibC::size_t strlcpy(char* dst, char const* src, LibC::size_t size);
extern "C" char* strcpy(char* dest, char const* src);

}
