#pragma once

#include <Kernel/Arch/x86_64/io.h>
#include <LibFK/Algorithms/log.h>

/**
 * @brief Non-Maskable Interrupt (NMI) controller
 *
 * This class provides static methods to enable or disable
 * NMIs via the CMOS/NMI port (0x70/0x71).
 */
class NMI {
public:
  /**
   * @brief Enable Non-Maskable Interrupts
   *
   * Clears the NMI-disable bit (bit 7) in port 0x70,
   * allowing NMIs to be delivered to the CPU.
   * Logs a message indicating that NMIs are enabled.
   */
  static void enable_nmi() {
    outb(0x70, inb(0x70) & 0x7F);
    fk::algorithms::klog("NMI", "Non maskable interrupt enabled");
  }

  /**
   * @brief Disable Non-Maskable Interrupts
   *
   * Sets the NMI-disable bit (bit 7) in port 0x70,
   * preventing NMIs from reaching the CPU.
   * Reads port 0x71 to flush the CMOS write.
   * Logs a message indicating that NMIs are disabled.
   */
  static void disable_nmi() {
    outb(0x70, inb(0x70) | 0x80);
    inb(0x71); // Flush CMOS
    fk::algorithms::klog("NMI", "Non maskable interrupt disabled");
  }
};
