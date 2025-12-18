#pragma once 

#include <Kernel/Hardware/Acpi/sdt_header.h>
#include <Kernel/Hardware/Fadt/generic_address_structures.h>
#include <LibFK/Types/types.h>

struct FADT {
    SDTHeader header; 

    uint32_t firmware_ctrl;
    uint32_t dsdt;

    uint8_t reserved1;

    uint8_t preferred_pm_profile;
    uint16_t sci_interrupt;
    uint32_t smi_command_port;
    uint8_t acpi_enable;
    uint8_t acpi_disable;

    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;

    uint8_t pm1_event_length;
    uint8_t pm1_control_length;

    uint32_t pm_timer_block;
    uint8_t pm_timer_length;

    uint16_t boot_arch_flags;

    uint8_t minor_version;

    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;

    GenericAddressStructure x_pm1a_event_block;
    GenericAddressStructure x_pm1a_control_block;
    GenericAddressStructure x_pm_timer_block;
}__attribute__((packed));