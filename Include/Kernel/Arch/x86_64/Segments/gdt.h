#pragma once

#include <Kernel/Arch/x86_64/arch_defs.h>
#include <LibC/stdint.h>
#include <LibFK/Algorithms/log.h>

alignas(16) static uint8_t ist_stacks[7][IST_STACK_SIZE];
alignas(16) static uint8_t rsp1_stack[IST_STACK_SIZE];
alignas(16) static uint8_t rsp2_stack[IST_STACK_SIZE];

struct GDTEntry {
  uint16_t limit_low;  // bits 0-15 do limit
  uint16_t base_low;   // bits 0-15 do base
  uint8_t base_middle; // bits 16-23 do base
  uint8_t access;      // flags de acesso
  uint8_t granularity; // flags de limite e tamanho
  uint8_t base_high;   // bits 24-31 do base
} __attribute__((packed));

struct TSSDescriptor {
  uint16_t limit_low;
  uint16_t base_low;
  uint8_t base_middle;
  uint8_t access;
  uint8_t granularity;
  uint8_t base_high;
  uint32_t base_upper; // base bits 32-63
  uint32_t reserved;
} __attribute__((packed));

struct GDTR {
  uint16_t limit;
  uint64_t base;
} __attribute__((packed));

struct [[gnu::packed]] TSS64 {
  uint32_t reserved0;
  uint64_t rsp0;
  uint64_t rsp1;
  uint64_t rsp2;
  uint64_t reserved1;
  uint64_t ist1;
  uint64_t ist2;
  uint64_t ist3;
  uint64_t ist4;
  uint64_t ist5;
  uint64_t ist6;
  uint64_t ist7;
  uint64_t reserved2;
  uint16_t reserved3;
  uint16_t io_map_base;
};

enum SegmentAccess : uint8_t {
  Ring0Code = 0x9A, // Present + ring0 + executable + readable
  Ring0Data = 0x92, // Present + ring0 + writable
  TSS64 = 0x89      // Present + 64-bit TSS
};

enum SegmentFlags : uint8_t { Granularity4K = 1 << 7, LongMode = 1 << 5 };

constexpr SegmentFlags operator|(SegmentFlags a, SegmentFlags b) {
  return static_cast<SegmentFlags>(static_cast<uint8_t>(a) |
                                   static_cast<uint8_t>(b));
}

constexpr SegmentAccess operator|(SegmentAccess a, SegmentAccess b) {
  return static_cast<SegmentAccess>(static_cast<uint8_t>(a) |
                                    static_cast<uint8_t>(b));
}

static constexpr uint64_t createSegment(SegmentAccess access,
                                        SegmentFlags flags) {
  uint64_t seg = 0;
  seg |= static_cast<uint64_t>(access) << 40;
  seg |= static_cast<uint64_t>(flags) << 48;
  seg |= 0xFFFFULL; // limit baixo
  return seg;
}

class GDTController {
private:
  bool m_initialized = false;

  uint64_t gdt[5] = {0};
  struct TSS64 tss = {};
  GDTR gdtr = {};

  void setupNull() { gdt[0] = 0; }

  void setupKernelCode() {
    gdt[1] =
        createSegment(SegmentAccess::Ring0Code,
                      SegmentFlags::LongMode | SegmentFlags::Granularity4K);
  }

  void setupKernelData() {
    gdt[2] =
        createSegment(SegmentAccess::Ring0Data, SegmentFlags::Granularity4K);
  }

  void setupTSS();
  void setupGDT();
  void setupGDTR();
  void loadGDT();
  void loadSegments();
  void loadTSS();

  GDTController() = default;

public:
  static GDTController &the() {
    static GDTController inst;
    return inst;
  }

  void initialize();
  void set_kernel_stack(uint64_t stack_addr);
};
