#pragma once

#include <Kernel/Hardware/Firmware/Acpi/sdt_header.h>
#include <Kernel/Hardware/Firmware/Fadt/generic_address_structures.h>

struct HPETTable {
    SDTHeader header;
    uint32_t event_timer_block_id;
    GenericAddressStructure base_address;
    uint8_t hpet_number;
    uint16_t main_counter_minimum_clock_tick_periodic;
    uint8_t page_protection_and_oem_attribute;
} __attribute__((packed));
