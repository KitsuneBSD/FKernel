#pragma once

#include <LibC/stddef.h>
#include <LibC/stdint.h>

#include <Kernel/Arch/x86_64/Cpu/Asm.h>
#include <Kernel/Arch/x86_64/Cpu/Constants.h>
#include <Kernel/Arch/x86_64/Segments/Tss.h>

namespace gdt {
enum class DescriptorIndex : LibC::uint8_t {
    Null = 0,
    KernelCode,
    KernelData,
    UserCode,
    UserData,
    FirstAvailableTSS = 5,
    Max
};

struct Descriptor {
    LibC::uint16_t limit_low;
    LibC::uint16_t base_low;
    LibC::uint8_t base_middle;
    LibC::uint8_t access;
    LibC::uint8_t granularity;
    LibC::uint8_t base_high;
} __attribute__((packed));

static_assert(sizeof(Descriptor) == 8, "GDT Descriptor must be 8 bytes");

struct GdtRegister {
    LibC::uint16_t limit;
    LibC::uint64_t base;
} __attribute__((packed));

static_assert(sizeof(GdtRegister) == 10, "GDT Register must be 10 bytes");

class Manager {
private:
    static constexpr LibC::size_t DescriptorCount = 8;

    Descriptor descriptors_[DescriptorCount];
    GdtRegister gdtr_ {};

    Tss::Tss tss_;

    alignas(16) static inline LibC::uint8_t ist_stack_nmi[4096];
    alignas(16) static inline LibC::uint8_t ist_stack_double_fault[4096];
    alignas(16) static inline LibC::uint8_t ist_stack_machine_check[4096];
    alignas(16) static inline LibC::uint64_t kernel_stack[16384];

    Manager() = default;
    Manager(Manager const&) = delete;
    Manager& operator=(Manager const&) = delete;

    void set_descriptor(int index, LibC::uint32_t base, LibC::uint32_t limit, LibC::uint8_t access, LibC::uint8_t flags) noexcept;
    void set_tss_descriptor(int index, LibC::uint64_t base, LibC::uint32_t limit) noexcept;

    void setup_tss() noexcept;
    void load_gdt() noexcept;
    void load_tss() noexcept;

public:
    static Manager& instance();

    void initialize() noexcept;
};

}
