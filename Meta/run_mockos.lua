#!/usr/bin/env lua

require("Meta.Lib.os_interact")
require("Meta.Lib.print_message")
require("Meta.Lib.run_command")

if not RunCommand("command -v qemu-system-x86_64 >/dev/null 2>&1") then
	PrintMessage(true, "qemu-system-x86_64 not found. Please install it before running this script.")
	os.exit(1)
end

local MockOS = "build/FKernel-MockOS.iso"
local HDA = "build/FKernel-HDA.qcow2"

if not FileExists(MockOS) or not FileExists(HDA) then
	RunCommand("xmake")
end

local qemu_cmd = {
	"qemu-system-x86_64",
	"-cdrom",
	MockOS,
	"-m",
	"2G",
	--"-nographic",
	--"-serial mon:stdio",
	"-smp 2",
	"-boot",
	"d",
}

local file = io.open(HDA, "r")
if file then
	file:close()
	table.insert(qemu_cmd, "-hda")
	table.insert(qemu_cmd, HDA)
	PrintMessage(false, "Starting QEMU with existing HDA disk")
else
	PrintMessage(false, "Starting QEMU with CD-ROM only")
end

local final_cmd = table.concat(qemu_cmd, " ")
RunCommand(final_cmd)
