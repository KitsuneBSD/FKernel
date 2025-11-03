#pragma once

#include <Kernel/Arch/x86_64/Interrupt/Handler/interrupt_frame.h>
#include <LibFK/Types/types.h>

/**
 * @brief Maximum number of entries in the x86_64 IDT
 */
constexpr size_t MAX_x86_64_IDT_SIZE = 256;

/**
 * @brief Interrupt Descriptor Table (IDT) entry for x86_64
 *
 * Each entry describes an interrupt or exception handler.
 * The structure is packed to match the CPU's expected format.
 */
struct idt_entry {
  uint16_t offset_low; ///< Lower 16 bits of the handler function address
  uint16_t selector;   ///< Code segment selector in GDT or LDT
  uint8_t ist;         ///< Interrupt Stack Table index (0 = default stack)
  uint8_t
      type_attr; ///< Type and attributes (e.g., gate type, DPL, present bit)
  uint16_t offset_mid;  ///< Middle 16 bits of the handler address
  uint32_t offset_high; ///< Higher 32 bits of the handler address
  uint32_t zero;        ///< Reserved, must be zero
} __attribute__((packed));

static_assert(sizeof(idt_entry) == 16, "IDT entry must be 16 bytes");

/**
 * @brief Pointer structure used to load the IDT with `lidt`
 */
struct idt_ptr {
  uint16_t limit; ///< Size of the IDT in bytes minus 1
  uint64_t base;  ///< Address of the first IDT entry
} __attribute__((packed));

/**
 * @brief Function pointer type for high-level interrupt handlers
 *
 * @param vector Interrupt vector number (0-255)
 * @param frame Pointer to the CPU state saved on the stack during the interrupt
 */
using interrupt = void (*)(uint8_t vector, InterruptFrame *frame);
