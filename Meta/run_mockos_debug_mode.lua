#!/usr/bin/env lua

-- Import libs
local os_interact = require("Meta.Lib.os_interact")
local PrintMessage = require("Meta.Lib.print_message")
local RunCommand = require("Meta.Lib.run_command")

-- Helpers
local function check_command(cmd, msg)
	if not RunCommand(cmd) then
		PrintMessage(true, msg)
		os.exit(1)
	end
end

local function file_exists(path)
	local f = io.open(path, "r")
	if f then
		f:close()
	end
	return f ~= nil
end

-- Check QEMU
check_command("command -v qemu-system-x86_64 >/dev/null 2>&1", "qemu-system-x86_64 not found. Please install it.")

-- Paths
local MOCK_OS = "build/FKernel-MockOS.iso"
local HDA = "build/FKernel-HDA.qcow2"
local LOG_DIR = "build/logs/"

-- Ensure log directory exists
os.execute("mkdir -p " .. LOG_DIR)

-- Build if missing
if not file_exists(MOCK_OS) or not file_exists(HDA) then
	RunCommand("xmake")
end

-- Base QEMU command
local qemu_cmd = {
	"qemu-system-x86_64",

	-- Machine & CPU
	"-machine",
	"pc", -- i440FX legado
	"-cpu",
	"core2duo",
	"-smp",
	"cores=2,threads=1",

	-- Memory
	"-m",
	"128M", -- limitado para simular OOM

	-- Boot
	"-cdrom",
	MOCK_OS,
	"-boot",
	"d",

	-- Devices
	"-serial",
	"file:" .. LOG_DIR .. "serial.log", -- serial principal em arquivo
	"-device",
	"pci-ohci,id=ohci",
	"-netdev",
	"user,id=n0",
	"-device",
	"e1000,netdev=n0",

	-- Display & debug
	"-nographic",
	"-no-reboot",
	"-no-shutdown",
	"-d",
	"int,guest_errors,mmu,pcall", -- debug detalhado
	"-D",
	LOG_DIR .. "qemu_debug.log", -- output debug em arquivo
}

-- Add HDA if exists
if file_exists(HDA) then
	table.insert(qemu_cmd, "-drive")
	table.insert(qemu_cmd, "file=" .. HDA .. ",if=ide,format=qcow2")
	PrintMessage(false, "Starting QEMU with existing HDA (IDE) disk")
else
	PrintMessage(false, "Starting QEMU with CD-ROM only (no HDA)")
end

-- Run final command
local final_cmd = table.concat(qemu_cmd, " ")
PrintMessage(false, "Running QEMU command (logs in logs/ folder):\n" .. final_cmd)
RunCommand(final_cmd)
