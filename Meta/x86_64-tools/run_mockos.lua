#!/usr/bin/env lua

require("Meta.Lib.os_interact")
require("Meta.Lib.print_message")
require("Meta.Lib.run_command")

local MockOS = "build/FKernel-MockOS.iso"
local HDA = "build/FKernel-HDA.qcow2"

function RunMockOS(debug_flags, output_log_path)
	if not OSInteract.CommandExists("qemu-system-x86_64") then
		PrintMessage(true, "qemu-system-x86_64 not found. Please install it before running this script.")
		return false
	end

	if not OSInteract.DirExists("logs") then
		RunCommand("mkdir -p logs/")
	end

	if not OSInteract.FileExists(MockOS) or not OSInteract.FileExists(HDA) then
		local ok = RunCommand("xmake")
		if not ok then
			PrintMessage(true, "xmake build failed — aborting.")
			return false
		end
	end

	local qemu_cmd = {
		"qemu-system-x86_64",
		"-cdrom " .. MockOS,
		"-m 2G",
		"-smp 2",
		"-boot d",
	}

	-- Determine if graphical mode or debug mode
	local is_debug_mode = debug_flags and output_log_path

	if is_debug_mode then
		table.insert(qemu_cmd, "-nographic")
		table.insert(qemu_cmd, "-d " .. debug_flags)
		table.insert(qemu_cmd, "-D " .. output_log_path)
		table.insert(qemu_cmd, "-serial mon:stdio")
	else
		table.insert(qemu_cmd, "-vga qxl")
		table.insert(qemu_cmd, "-d int,mmu")
		table.insert(qemu_cmd, "-serial file:logs/serial.log")
	end

	if OSInteract.FileExists(HDA) then
		table.insert(qemu_cmd, "-hda " .. HDA)
		PrintMessage(false, "Starting QEMU with existing HDA disk")
	else
		PrintMessage(false, "Starting QEMU with CD-ROM only")
	end

	local final_cmd = table.concat(qemu_cmd, " ")
	PrintMessage(false, "Executing QEMU command: " .. final_cmd)
	return RunCommand(final_cmd)
end

-- If this script is run directly (not required as a module), it will run QEMU in graphical mode
-- This ensures existing 'xmake run' functionality remains similar
if not ... then
	RunMockOS()
end

return RunMockOS
