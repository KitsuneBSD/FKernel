#pragma once

#include <LibC/stdint.h>
#include <LibC/stdio.h>

// NOTE: We need implement a function for make a hash in this traits, i think in
// use DJB2 on global hash (for any arquicteture). But in enviroments x64 use
// Crc32

template <typename T> struct Traits {};

template <> struct Traits<int> {
  // TODO: Use a Real Hash function DJB2 or Crc32
  static unsigned hash(int i) { return i; }
  static void dump(int i) { kprintf("%d", i); }
};

template <> struct Traits<unsigned> {
  // TODO: Use a Real Hash function DJB2 or Crc32
  static unsigned hash(unsigned i) { return i; }
  static void dump(unsigned i) { kprintf("%d", i); }
};
