#pragma once

#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/log.h>

class NMI {
public:
  static void enable_nmi() {
    outb(0x70, inb(0x70) & 0x7F);
    klog("NMI", "Non maskable interrupt enabled");
  }
  static void disable_nmi() {
    outb(0x70, inb(0x70) | 0x80);
    inb(0x71);
    klog("NMI", "Non maskable interrupt disabled");
  }
};
