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
    uint8_t s4bios_req;
    uint8_t pstate_cnt;
    uint32_t pm1a_event_block;
    uint32_t pm1b_event_block;
    uint32_t pm1a_control_block;
    uint32_t pm1b_control_block;
    uint32_t pm2_control_block;
    uint32_t pm_timer_block;
    uint32_t gpe0_block;
    uint32_t gpe1_block;
    uint8_t pm1_event_length;
    uint8_t pm1_control_length;
    uint8_t pm2_control_length;
    uint8_t pm_timer_length;
    uint8_t gpe0_block_length;
    uint8_t gpe1_block_length;
    uint8_t gpe1_base;
    uint8_t cst_cnt;
    uint16_t p_lvl2_lat;
    uint16_t p_lvl3_lat;
    uint16_t flush_size;
    uint16_t flush_stride;
    uint8_t duty_offset;
    uint8_t duty_width;
    uint8_t day_alrm;
    uint8_t mon_alrm;
    uint8_t century;
    uint16_t iapc_boot_arch;
    uint8_t reserved2;
    uint32_t flags;
    GenericAddressStructure reset_reg;
    uint8_t reset_value;
    uint16_t arm_boot_arch;
    uint8_t fadt_minor_version;

    uint64_t x_firmware_ctrl;
    uint64_t x_dsdt;
    GenericAddressStructure x_pm1a_event_block;
    GenericAddressStructure x_pm1b_event_block;
    GenericAddressStructure x_pm1a_control_block;
    GenericAddressStructure x_pm1b_control_block;
    GenericAddressStructure x_pm2_timer_block;
    GenericAddressStructure x_gpe0_block;
    GenericAddressStructure x_gpe1_block;
    GenericAddressStructure x_pm_timer_block;
}__attribute__((packed));
