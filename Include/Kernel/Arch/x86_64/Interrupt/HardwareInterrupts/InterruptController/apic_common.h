#pragma once

#include <LibFK/Types/types.h>

// APIC Base MSR — shared between xAPIC and x2APIC
constexpr uint32_t APIC_BASE_MSR         = 0x1B;
constexpr uint32_t APIC_MSR_ENABLE       = 1u << 11;
constexpr uint32_t APIC_MSR_X2APIC_MODE = 1u << 10;
constexpr uintptr_t APIC_RANGE_SIZE      = 0x1000;
// Standard LAPIC physical base per Intel SDM; used as fallback if ACPI/MSR query fails
constexpr uintptr_t LAPIC_DEFAULT_PHYS_BASE = 0xFEE00000;

// Shared LVT/SVR constants
constexpr uint8_t  APIC_SPURIOUS_VECTOR         = 0xFF;
constexpr uint8_t  APIC_TIMER_VECTOR            = 0x20;
constexpr uint32_t APIC_SVR_ENABLE              = 1u << 8;
constexpr uint32_t APIC_LVT_MASK               = 1u << 16;
constexpr uint32_t APIC_LVT_TIMER_MODE_PERIODIC = 1u << 17;
constexpr uint32_t APIC_TIMER_DIVISOR           = 0x3; // divide by 16

// xAPIC MMIO register offsets
constexpr uint32_t APIC_REG_EOI           = 0x0B0;
constexpr uint32_t APIC_REG_SPURIOUS      = 0x0F0;
constexpr uint32_t APIC_REG_LVT_TIMER    = 0x320;
constexpr uint32_t APIC_REG_INITIAL_COUNT = 0x380;
constexpr uint32_t APIC_REG_CURRENT_COUNT = 0x390;
constexpr uint32_t APIC_REG_DIVIDE_CONFIG = 0x3E0;

// x2APIC MSR addresses
constexpr uint32_t X2APIC_EOI_MSR           = 0x80B;
constexpr uint32_t X2APIC_SPURIOUS_MSR      = 0x80F;
constexpr uint32_t X2APIC_LVT_TIMER_MSR     = 0x832;
constexpr uint32_t X2APIC_INITIAL_COUNT_MSR = 0x838;
constexpr uint32_t X2APIC_CURRENT_COUNT_MSR = 0x839;
constexpr uint32_t X2APIC_DIVIDE_CONFIG_MSR = 0x83E;
