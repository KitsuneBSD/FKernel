#pragma once

#include <LibC/stdint.h>

constexpr LibC::uint16_t PRIMARY_IO_BASE = 0x1F0;
constexpr LibC::uint16_t PRIMARY_CTRL_BASE = 0x3F6;
constexpr LibC::uint16_t SECONDARY_IO_BASE = 0x170;
constexpr LibC::uint16_t SECONDARY_CTRL_BASE = 0x376;

constexpr LibC::uint16_t ATA_REG_DATA = 0x00;
constexpr LibC::uint16_t ATA_REG_ERROR = 0x01;
constexpr LibC::uint16_t ATA_REG_SECCOUNT = 0x02;
constexpr LibC::uint16_t ATA_REG_LBA_LOW = 0x03;
constexpr LibC::uint16_t ATA_REG_LBA_MID = 0x04;
constexpr LibC::uint16_t ATA_REG_LBA_HIGH = 0x05;
constexpr LibC::uint16_t ATA_REG_DEVICE = 0x06;
constexpr LibC::uint16_t ATA_REG_COMMAND = 0x07;
constexpr LibC::uint16_t ATA_REG_STATUS = 0x07;

constexpr LibC::uint8_t ATA_CMD_IDENTIFY = 0xEC;
