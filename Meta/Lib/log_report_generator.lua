#!/usr/bin/env lua

local LogFileIO = require("Meta.Lib.log_file_io")
require("Meta.Lib.log_parser")

local LogReportGenerator = {}

function LogReportGenerator.output_timestamps(timestamps, gaps, outdir)
	local path = outdir .. "/08_timestamps.txt"
	local buf = {}

	table.insert(buf, "=== TIMESTAMP ANALYSIS ===\n")

	table.insert(buf, "Total timestamps: " .. #timestamps .. "\n")
	table.insert(buf, "Detected gaps: " .. #gaps .. "\n\n")

	table.insert(buf, "--- Gaps > threshold ---\n")
	for _, g in ipairs(gaps) do
		table.insert(buf, string.format("Line %d -> Line %d | Δt = %.3f sec\n", g.prev_line, g.curr_line, g.delta))
	end

	LogFileIO.write_file(path, table.concat(buf))
end

function LogReportGenerator.output_silence_periods(silences, outdir)
	local path = outdir .. "/09_silence_period.txt"
	local buf = {}

	table.insert(buf, "=== SILENCE DETECTOR ===\n")
	table.insert(buf, "Detected silence blocks: " .. #silences .. "\n\n")

	for _, s in ipairs(silences) do
		table.insert(buf, string.format("Silence: lines %d → %d (%d lines)\n", s.start_line, s.end_line, s.length))
	end

	LogFileIO.write_file(path, table.concat(buf))
end

function LogReportGenerator.output_interrupt_storms(storms, outdir)
	local path = outdir .. "/10_interrupt_storms.txt"
	local buf = {}

	table.insert(buf, "=== INTERRUPT STORM DETECTOR ===\n")
	table.insert(buf, "Detected storms: " .. #storms .. "\n\n")

	for _, s in ipairs(storms) do
		table.insert(buf, string.format("Vector %d storm: %d events around line %d\n", s.vector, s.count, s.line))
	end

	LogFileIO.write_file(path, table.concat(buf))
end

function LogReportGenerator.output_incomplete_init(list, outdir)
	local path = outdir .. "/11_incomplete_init.txt"
	local buf = {}

	table.insert(buf, "=== SUBSYSTEM INITIALIZATION ISSUES ===\n")
	table.insert(buf, "Incomplete subsystems: " .. #list .. "\n\n")

	for _, entry in ipairs(list) do
		table.insert(buf, string.format("- %s (first mention at line %d)\n", entry.name, entry.first_line or -1))
	end

	LogFileIO.write_file(path, table.concat(buf))
end

function LogReportGenerator.output_stack_failures(fails, outdir)
	local path = outdir .. "/12_stack_failures.txt"
	local buf = {}

	table.insert(buf, "=== STACK FAILURES ===\n")
	table.insert(buf, "Detected: " .. #fails .. "\n\n")

	for _, s in ipairs(fails) do
		table.insert(buf, string.format("Line %d: %s\n", s.line_num, s.content))
	end

	LogFileIO.write_file(path, table.concat(buf))
end

function LogReportGenerator.output_mmu_failures(list, outdir)
	local path = outdir .. "/13_mmu_failures.txt"
	local buf = {}

	table.insert(buf, "=== MMU / PAGING FAILURES ===\n")
	table.insert(buf, "Detected: " .. #list .. "\n\n")

	for _, m in ipairs(list) do
		table.insert(buf, string.format("Line %d: %s\n", m.line_num, m.content))
	end

	LogFileIO.write_file(path, table.concat(buf))
end

function LogReportGenerator.output_errors(errors, output_dir)
	local content = string.format(
		[[ 
================================================================================
                          CRITICAL ERRORS REPORT
================================================================================
Total errors found: %d
Generated: %s
================================================================================

]],
		#errors,
		os.date("%Y-%m-%d %H:%M:%S")
	)

	if #errors == 0 then
		content = content .. "✓ No critical errors detected!\n"
	else
		for i, err in ipairs(errors) do
			content = content
				.. string.format(
					"[%d] Line %d - Pattern: %s - Severity: %s\n%s\n\n",
					i,
					err.line_num,
					err.pattern,
					err.severity,
					err.content
				)
		end
	end

	LogFileIO.write_file(output_dir .. "/01_errors.txt", content)
	PrintMessage(false, "✓ Generated: 01_errors.txt (" .. #errors .. " errors)")
end

function LogReportGenerator.output_warnings(warnings, output_dir)
	local content = string.format(
		[[ 
================================================================================
                            WARNINGS REPORT
================================================================================
Total warnings found: %d
Generated: %s
================================================================================

]],
		#warnings,
		os.date("%Y-%m-%d %H:%M:%S")
	)

	if #warnings == 0 then
		content = content .. "✓ No warnings detected!\n"
	else
		for i, warn in ipairs(warnings) do
			content = content .. string.format("[%d] Line %d\n%s\n\n", i, warn.line_num, warn.content)
		end
	end

	LogFileIO.write_file(output_dir .. "/02_warnings.txt", content)
	PrintMessage(false, "✓ Generated: 02_warnings.txt (" .. #warnings .. " warnings)")
end

function LogReportGenerator.output_smm(smm_data, output_dir)
	local content = string.format(
		[[ 
================================================================================
                        SMM TRANSITIONS REPORT
================================================================================
SMM Enter count: %d
SMM Exit count:  %d
Balance: %s
Generated: %s
================================================================================

]],
		smm_data.enter,
		smm_data.exit,
		(smm_data.enter == smm_data.exit) and "✓ BALANCED" or "✗ UNBALANCED",
		os.date("%Y-%m-%d %H:%M:%S")
	)

	content = content .. "\nAll transitions:\n\n"

	for i, trans in ipairs(smm_data.transitions) do
		content = content
			.. string.format("[%d] Line %d - Type: %s\n%s\n\n", i, trans.line_num, trans.type:upper(), trans.content)
	end

	LogFileIO.write_file(output_dir .. "/03_smm_transitions.txt", content)
	PrintMessage(false, "✓ Generated: 03_smm_transitions.txt (" .. smm_data.enter .. " transitions)")
end

function LogReportGenerator.output_leaks(leaks, output_dir)
	local content = string.format(
		[[ 
================================================================================
                         MEMORY LEAKS REPORT
================================================================================
Total leaks detected: %d
Generated: %s
================================================================================

]],
		#leaks,
		os.date("%Y-%m-%d %H:%M:%S")
	)

	if #leaks == 0 then
		content = content .. "✓ No memory leaks detected!\n"
	else
		for i, leak in ipairs(leaks) do
			content = content
				.. string.format(
					"[%d] Line %d - Pointer: %s\n%s\n\n",
					i,
					leak.line_num,
					leak.ptr or "unknown",
					leak.content
				)
		end
	end

	LogFileIO.write_file(output_dir .. "/04_memory_leaks.txt", content)
	PrintMessage(false, "✓ Generated: 04_memory_leaks.txt (" .. #leaks .. " leaks)")
end

function LogReportGenerator.output_subsystems(subsystems, output_dir)
	local content = string.format(
		[[ 
================================================================================
                      SUBSYSTEMS INITIALIZATION STATUS
================================================================================
Generated: %s
================================================================================

]],
		os.date("%Y-%m-%d %H:%M:%S")
	)

	content = content .. "SUMMARY:\n\n"
	content = content .. string.format("% -25s % -15s % -10s %s\n", "SUBSYSTEM", "INITIALIZED", "SUCCESS", "LINES")
	content = content .. string.rep("-", 80) .. "\n"

	local sorted = {}
	for name, status in pairs(subsystems) do
		table.insert(sorted, { name = name, status = status })
	end
	table.sort(sorted, function(a, b)
		return a.name < b.name
	end)

	for _, item in ipairs(sorted) do
		local status = item.status
		local symbol_init = status.initialized and "✓" or "✗"
		local symbol_success = status.success and "✓" or "✗"
		local line_range = status.first_line
				and string.format("%d-%d", status.first_line, status.last_line or status.first_line)
			or "N/A"

		content = content
			.. string.format("% -25s % -15s % -10s %s\n", item.name, symbol_init, symbol_success, line_range)
	end

	content = content .. "\n\n" .. string.rep("=", 80) .. "\n"
	content = content .. "DETAILED MESSAGES:\n"
	content = content .. string.rep("=", 80) .. "\n\n"

	for _, item in ipairs(sorted) do
		if item.status.initialized then
			content = content .. "\n[" .. item.name .. "]:\n"
			content = content .. string.rep("-", 80) .. "\n"
			for _, msg in ipairs(item.status.messages) do
				content = content .. string.format("  Line %d: %s\n", msg.line_num, msg.content)
			end
			content = content .. "\n"
		end
	end

	LogFileIO.write_file(output_dir .. "/05_subsystems.txt", content)
	PrintMessage(false, "✓ Generated: 05_subsystems.txt")
end

function LogReportGenerator.output_statistics(log_types, total_lines, output_dir)
	local content = string.format(
		[[ 
================================================================================
                          LOG STATISTICS REPORT
================================================================================
Total lines analyzed: %d
Generated: %s
================================================================================

]],
		total_lines,
		os.date("%Y-%m-%d %H:%M:%S")
	)

	local sorted = {}
	for type_name, count in pairs(log_types) do
		table.insert(sorted, { name = type_name, count = count })
	end
	table.sort(sorted, function(a, b)
		return a.count > b.count
	end)

	content = content .. string.format("% -5s % -30s % -10s %s\n", "RANK", "LOG TYPE", "COUNT", "PERCENTAGE")
	content = content .. string.rep("-", 80) .. "\n"

	for i, item in ipairs(sorted) do
		local percentage = (item.count / total_lines) * 100
		content = content
			.. string.format("% -5d % -30s % -10d %.2f%%\n", i, "[" .. item.name .. "]", item.count, percentage)
	end

	LogFileIO.write_file(output_dir .. "/06_statistics.txt", content)
	PrintMessage(false, "✓ Generated: 06_statistics.txt (" .. #sorted .. " log types)")
end

function LogReportGenerator.output_interrupts(int_data, output_dir)
	local content = string.format(
		[[ 
================================================================================
                        INTERRUPT ACTIVITY REPORT
================================================================================
Total interrupt events: %d
Generated: %s
================================================================================

]],
		int_data.total,
		os.date("%Y-%m-%d %H:%M:%S")
	)

	if int_data.total > 0 then
		content = content .. "INTERRUPT VECTORS DISTRIBUTION:\n\n"
		content = content .. string.format("% -10s % -15s %s\n", "VECTOR", "COUNT", "DESCRIPTION")
		content = content .. string.rep("-", 80) .. "\n"

		local sorted = {}
		for vector, count in pairs(int_data.vectors) do
			table.insert(sorted, { vector = vector, count = count })
		end
		table.sort(sorted, function(a, b)
			return a.count > b.count
		end)

		local descriptions = {
			[0] = "Divide Error",
			[1] = "Debug",
			[2] = "NMI",
			[3] = "Breakpoint",
			[8] = "Double Fault",
			[13] = "General Protection",
			[14] = "Page Fault",
			[32] = "Timer (IRQ0)",
			[33] = "Keyboard (IRQ1)",
			[40] = "RTC (IRQ8)",
			[46] = "Primary IDE (IRQ14)",
			[47] = "Secondary IDE (IRQ15)",
		}

		for _, item in ipairs(sorted) do
			local desc = descriptions[item.vector] or "Unknown"
			content = content .. string.format("% -10d % -15d %s\n", item.vector, item.count, desc)
		end
	else
		content = content .. "No interrupt activity detected.\n"
	end

	LogFileIO.write_file(output_dir .. "/07_interrupts.txt", content)
	PrintMessage(false, "✓ Generated: 07_interrupts.txt (" .. int_data.total .. " events)")
end

function LogReportGenerator.output_last_lines(last_lines, output_dir)
	local content = string.format(
		[[ 
================================================================================
                          LAST 100 LINES OF LOG
================================================================================
Generated: %s
================================================================================

]],
		os.date("%Y-%m-%d %H:%M:%S")
	)

	for _, item in ipairs(last_lines) do
		content = content .. string.format("[%d] %s\n", item.line_num, item.content)
	end

	LogFileIO.write_file(output_dir .. "/08_last_lines.txt", content)
	PrintMessage(false, "✓ Generated: 08_last_lines.txt")
end

function LogReportGenerator.output_summary(data, output_dir)
	local content = string.format(
		[[ 
================================================================================
                         ANALYSIS SUMMARY REPORT
================================================================================
Generated: %s
Log file: %s
Total lines: %d
================================================================================

CRITICAL FINDINGS:
------------------
Errors:           %d  %s
Warnings:         %d  %s
Memory Leaks:     %d  %s
SMM Transitions:  %d enter, %d exit %s

SUBSYSTEMS STATUS:
------------------
]],
		os.date("%Y-%m-%d %H:%M:%S"),
		data.filename,
		data.total_lines,
		data.errors_count,
		data.errors_count > 0 and "⚠" or "✓",
		data.warnings_count,
		data.warnings_count > 0 and "⚠" or "✓",
		data.leaks_count,
		data.leaks_count > 0 and "⚠" or "✓",
		data.smm_enter,
		data.smm_exit,
		(data.smm_enter == data.smm_exit) and "✓" or "✗"
	)

	local success_count = 0
	local failed_count = 0
	local total_subsystems = 0

	for _, status in pairs(data.subsystems) do
		total_subsystems = total_subsystems + 1
		if status.success then
			success_count = success_count + 1
		elseif status.initialized then
			failed_count = failed_count + 1
		end
	end

	content = content
		.. string.format(
			[[ 
Total Subsystems: %d
Successfully Initialized: %d
Failed/Incomplete: %d
Not Found: %d

TOP 5 LOG TYPES:
----------------
]],
			total_subsystems,
			success_count,
			failed_count,
			total_subsystems - success_count - failed_count
		)

	local sorted = {}
	for name, count in pairs(data.log_types) do
		table.insert(sorted, { name = name, count = count })
	end
	table.sort(sorted, function(a, b)
		return a.count > b.count
	end)

	for i = 1, math.min(5, #sorted) do
		content = content .. string.format("%d. [%s]: %d occurrences\n", i, sorted[i].name, sorted[i].count)
	end

	content = content .. [[ 

 RECOMMENDATIONS:
----------------
]]

	if data.errors_count > 0 then
		content = content .. "- Review errors.txt for critical issues\n"
	end
	if data.warnings_count > 0 then
		content = content .. "- Check warnings.txt for potential problems\n"
	end
	if data.leaks_count > 0 then
		content = content .. "- Investigate memory leaks in memory_leaks.txt\n"
	end
	if data.smm_enter ~= data.smm_exit then
		content = content .. "- SMM transitions are unbalanced - check smm_transitions.txt\n"
	end
	if success_count < total_subsystems then
		content = content .. "- Some subsystems failed initialization - see subsystems.txt\n"
	end
	if data.errors_count == 0 and data.warnings_count == 0 then
		content = content .. "✓ No critical issues detected - system appears healthy\n"
	end

	content = content .. "\n" .. string.rep("=", 80) .. "\n"
	content = content .. "End of summary report.\n"

	LogFileIO.write_file(output_dir .. "/00_summary.txt", content)
	PrintMessage(false, "✓ Generated: 00_summary.txt")
end

return LogReportGenerator
