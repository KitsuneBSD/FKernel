#!/usr/bin/env lua

require("Meta.Lib.run_command")
require("Meta.Lib.os_interact")
require("Meta.Lib.print_message")
require("Meta.x86_64-tools.run_mockos")

local QemuRunner = {}

-- Function to run QEMU and capture its output
-- qemu_debug_flags: QEMU debug flags (e.g., "int,mmu,cpu")
-- output_log_path: Path where QEMU's debug log will be written
function QemuRunner.run_qemu_and_capture_log(qemu_debug_flags, output_log_path)
	PrintMessage(false, "Ensuring kernel is built...")
	local build_ok = RunCommand("xmake")
	if not build_ok then
		PrintMessage(true, "Kernel build failed. Aborting QEMU run.")
		return false
	end

	PrintMessage(false, "Starting QEMU with debug flags: " .. qemu_debug_flags)
	PrintMessage(false, "QEMU debug log will be written to: " .. output_log_path)

	-- Call the RunMockOS function from the run_mockos module
	return RunMockOS(qemu_debug_flags, output_log_path)
end

return QemuRunner
