#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibFK/Algorithms/log.h>
#include <LibFK/Types/types.h>

/**
 * @brief Statically allocated Interrupt Stack Table (IST) stacks
 *
 * Aligns stacks to 16 bytes for x86_64 ABI compliance.
 */
alignas(16) static uint8_t ist_stacks[7][IST_STACK_SIZE];
alignas(16) static uint8_t rsp1_stack[IST_STACK_SIZE];
alignas(16) static uint8_t rsp2_stack[IST_STACK_SIZE];

/**
 * @brief Standard Global Descriptor Table (GDT) entry
 */
struct GDTEntry {
  uint16_t limit_low;  ///< Lower 16 bits of the segment limit
  uint16_t base_low;   ///< Lower 16 bits of the base address
  uint8_t base_middle; ///< Middle 8 bits of the base address
  uint8_t access;      ///< Access flags (present, ring, executable, etc.)
  uint8_t granularity; ///< Granularity and segment size flags
  uint8_t base_high;   ///< High 8 bits of the base address
} __attribute__((packed));

/**
 * @brief Task State Segment (TSS) descriptor for GDT
 */
struct TSSDescriptor {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
  uint32_t base_upper; ///< Base bits 32-63
  uint32_t reserved;
} __attribute__((packed));

/**
 * @brief GDT pointer used by `lgdt` instruction
 */
struct GDTR {
  uint16_t limit; ///< Size of GDT in bytes minus 1
  uint64_t base;  ///< Address of the first GDT entry
} __attribute__((packed));

/**
 * @brief 64-bit Task State Segment (TSS) structure
 */
struct [[gnu::packed]] TSS64 {
  uint32_t reserved0;
  uint64_t rsp0; ///< Ring 0 stack pointer
  uint64_t rsp1; ///< Ring 1 stack pointer
  uint64_t rsp2; ///< Ring 2 stack pointer
  uint64_t reserved1;
  uint64_t ist1; ///< Interrupt Stack Table entry 1
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t io_map_base; ///< I/O permission bitmap offset
};

/**
 * @brief Segment access types
 */
enum SegmentAccess : uint8_t {
  Ring0Code = 0x9A, ///< Present + ring0 + executable + readable
  Ring0Data = 0x92, ///< Present + ring0 + writable
  TSS64 = 0x89      ///< Present + 64-bit TSS
};

/**
 * @brief Segment flags
 */
enum SegmentFlags : uint8_t {
  Granularity4K = 1 << 7, ///< 4K-byte granularity
  LongMode = 1 << 5       ///< 64-bit segment
};

/**
 * @brief Bitwise OR operator for SegmentFlags
 */
constexpr SegmentFlags operator|(SegmentFlags a, SegmentFlags b) {
  return static_cast<SegmentFlags>(static_cast<uint8_t>(a) |
                                   static_cast<uint8_t>(b));
}

/**
 * @brief Bitwise OR operator for SegmentAccess
 */
constexpr SegmentAccess operator|(SegmentAccess a, SegmentAccess b) {
  return static_cast<SegmentAccess>(static_cast<uint8_t>(a) |
                                    static_cast<uint8_t>(b));
}

/**
 * @brief Helper to create a 64-bit GDT segment descriptor
 *
 * @param access Segment access flags
 * @param flags Segment flags (granularity, long mode)
 * @return 64-bit encoded GDT entry
 */
static constexpr uint64_t createSegment(SegmentAccess access,
                                        SegmentFlags flags) {
  uint64_t seg = 0;
  seg |= static_cast<uint64_t>(access) << 40;
  seg |= static_cast<uint64_t>(flags) << 48;
  seg |= 0xFFFFULL; // low 16 bits of limit
  return seg;
}

/**
 * @brief Global Descriptor Table (GDT) controller
 *
 * This singleton class manages the GDT, TSS, and associated stacks.
 * It provides initialization and allows setting the kernel stack pointer.
 */
class GDTController {
private:
  bool m_initialized = false; ///< Tracks whether GDT is initialized

  uint64_t gdt[6] = {0}; ///< Array of GDT entries
  struct TSS64 tss = {}; ///< TSS structure
  GDTR gdtr = {};        ///< GDTR structure

  void setupNull();       ///< Setup the null descriptor
  void setupKernelCode(); ///< Setup kernel code segment
  void setupKernelData(); ///< Setup kernel data segment
  void setupTSS();        ///< Setup TSS descriptor
  void setupGDT();        ///< Setup all GDT entries
  void setupGDTR();       ///< Setup GDTR structure
  void loadGDT();         ///< Load GDT using lgdt
  void loadSegments();    ///< Load segment registers
  void loadTSS();         ///< Load TSS using ltr

  GDTController() = default; ///< Private constructor for singleton

public:
  /**
   * @brief Get the singleton instance of the GDTController
   * @return Reference to GDTController
   */
  static GDTController &the() {
    static GDTController inst;
    return inst;
  }

  /**
   * @brief Initialize the GDT and TSS
   */
  void initialize();

  /**
   * @brief Set the kernel stack pointer for Ring 0
   *
   * @param stack_addr Physical address of the stack
   */
  void set_kernel_stack(uint64_t stack_addr);
};
