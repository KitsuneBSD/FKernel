#!/usr/bin/env lua

require("Meta.Lib.os_interact")
require("Meta.Lib.print_message")
require("Meta.Lib.run_command")

local is_graphical_mode = true

if not CommandExists("qemu-system-x86_64") then
	PrintMessage(true, "qemu-system-x86_64 not found. Please install it before running this script.")
	os.exit(1)
end

if not FileExists("logs") then
	RunCommand("mkdir -p logs/")
end

local MockOS = "build/FKernel-MockOS.iso"
local HDA = "build/FKernel-HDA.raw"

if not FileExists(MockOS) or not FileExists(HDA) then
	local ok = RunCommand("xmake")
	if not ok then
		PrintMessage(true, "xmake build failed — aborting.")
		os.exit(1)
	end
end

local qemu_cmd = {
	"qemu-system-x86_64",
	"-cdrom " .. MockOS,
	"-m 2G",
	"-smp 2",
	"-boot d",
}

if is_graphical_mode then
	table.insert(qemu_cmd, "-vga qxl")
	table.insert(qemu_cmd, "-serial file:logs/serial.log")
else
	table.insert(qemu_cmd, "-nographic")
	table.insert(qemu_cmd, "-serial mon:stdio")
end

if FileExists(HDA) then
	table.insert(qemu_cmd, "-hda " .. HDA)
	PrintMessage(false, "Starting QEMU with existing HDA disk")
else
	PrintMessage(false, "Starting QEMU with CD-ROM only")
end

local final_cmd = table.concat(qemu_cmd, " ")
RunCommand(final_cmd)
