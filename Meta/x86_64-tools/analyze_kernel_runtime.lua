#!/usr/bin/env lua

local LogAnalyzer = {}

function LogAnalyzer.ensure_directory(path)
	os.execute("mkdir -p " .. path)
end

function LogAnalyzer.write_file(filename, content)
	local file = io.open(filename, "w")
	if not file then
		print("ERROR: Could not write to " .. filename)
		return false
	end
	file:write(content)
	file:close()
	return true
end

function LogAnalyzer.read_log(filename)
	local lines = {}
	local file = io.open(filename, "r")

	if not file then
		print("ERROR: Could not open file " .. filename)
		return nil
	end

	for line in file:lines() do
		table.insert(lines, line)
	end

	file:close()
	print("✓ File loaded: " .. #lines .. " lines")
	return lines
end

function LogAnalyzer.extract_log_type(line)
	local log_type = line:match("%[([^%]]+)%]")
	return log_type
end

function LogAnalyzer.detect_severity(line)
	if line:match("%[31m") then
		return "ERROR"
	elseif line:match("%[33m") then
		return "WARNING"
	elseif line:match("%[32m") then
		return "SUCCESS"
	elseif line:match("%[37m") then
		return "DEBUG"
	else
		return "INFO"
	end
end

function LogAnalyzer.strip_ansi(line)
	return line:gsub("\27%[[%d;]*m", "")
end

function LogAnalyzer.find_errors(lines)
	local errors = {}
	local error_patterns = {
		"fault",
		"panic",
		"error",
		"fail",
		"exception",
		"triple",
		"double fault",
		"page fault",
		"gpf",
		"crashed",
		"abort",
		"fatal",
		"invalid",
	}

	for i, line in ipairs(lines) do
		local line_lower = line:lower()
		for _, pattern in ipairs(error_patterns) do
			if line_lower:find(pattern) then
				table.insert(errors, {
					line_num = i,
					content = LogAnalyzer.strip_ansi(line),
					pattern = pattern,
					severity = LogAnalyzer.detect_severity(line),
				})
				break
			end
		end
	end

	return errors
end

function LogAnalyzer.find_warnings(lines)
	local warnings = {}

	for i, line in ipairs(lines) do
		local severity = LogAnalyzer.detect_severity(line)
		if severity == "WARNING" or line:lower():find("warning") then
			table.insert(warnings, {
				line_num = i,
				content = LogAnalyzer.strip_ansi(line),
			})
		end
	end

	return warnings
end

function LogAnalyzer.count_smm_transitions(lines)
	local smm_enter = 0
	local smm_exit = 0
	local transitions = {}

	for i, line in ipairs(lines) do
		if line:find("SMM: enter") then
			smm_enter = smm_enter + 1
			table.insert(transitions, {
				line_num = i,
				type = "enter",
				content = LogAnalyzer.strip_ansi(line),
			})
		elseif line:find("SMM: after RSM") then
			smm_exit = smm_exit + 1
			table.insert(transitions, {
				line_num = i,
				type = "exit",
				content = LogAnalyzer.strip_ansi(line),
			})
		end
	end

	return {
		enter = smm_enter,
		exit = smm_exit,
		transitions = transitions,
	}
end

function LogAnalyzer.count_log_types(lines)
	local counts = {}

	for _, line in ipairs(lines) do
		local log_type = LogAnalyzer.extract_log_type(line)
		if log_type then
			counts[log_type] = (counts[log_type] or 0) + 1
		end
	end

	return counts
end

function LogAnalyzer.find_memory_leaks(lines)
	local leaks = {}

	for i, line in ipairs(lines) do
		if line:find("leaked ptr") or line:find("leak") then
			local ptr = line:match("ptr=(0x%x+)")
			table.insert(leaks, {
				line_num = i,
				ptr = ptr,
				content = LogAnalyzer.strip_ansi(line),
			})
		end
	end

	return leaks
end

function LogAnalyzer.check_subsystem_init(lines)
	local subsystems = {
		"GDT",
		"IDT",
		"INTERRUPT",
		"APIC",
		"IOAPIC",
		"PIC",
		"TIMER",
		"RTC",
		"PHYSICAL MEMORY",
		"VIRTUAL MEMORY",
		"ACPI",
		"TSS",
		"NMI",
		"CPU",
		"CLOCK MANAGER",
	}

	local status = {}

	for _, subsystem in ipairs(subsystems) do
		status[subsystem] = {
			initialized = false,
			success = false,
			first_line = nil,
			last_line = nil,
			messages = {},
		}
	end

	for i, line in ipairs(lines) do
		for _, subsystem in ipairs(subsystems) do
			if line:find("%[" .. subsystem .. "%]") then
				if not status[subsystem].initialized then
					status[subsystem].initialized = true
					status[subsystem].first_line = i
				end

				status[subsystem].last_line = i
				table.insert(status[subsystem].messages, {
					line_num = i,
					content = LogAnalyzer.strip_ansi(line),
				})

				if line:find("initialized") or line:find("complete") or line:find("enabled") or line:find("set to") then
					local severity = LogAnalyzer.detect_severity(line)
					if severity == "SUCCESS" or line:match("%[32m") then
						status[subsystem].success = true
					end
				end
			end
		end
	end

	return status
end

function LogAnalyzer.search_pattern(lines, pattern, case_sensitive)
	local results = {}

	for i, line in ipairs(lines) do
		local search_line = case_sensitive and line or line:lower()
		local search_pattern = case_sensitive and pattern or pattern:lower()

		if search_line:find(search_pattern) then
			table.insert(results, {
				line_num = i,
				content = LogAnalyzer.strip_ansi(line),
			})
		end
	end

	return results
end

function LogAnalyzer.get_last_lines(lines, n)
	local result = {}
	local start = math.max(1, #lines - n + 1)

	for i = start, #lines do
		table.insert(result, {
			line_num = i,
			content = LogAnalyzer.strip_ansi(lines[i]),
		})
	end

	return result
end

function LogAnalyzer.detect_loops(lines, threshold)
	threshold = threshold or 10
	local consecutive = {}
	local loops = {}
	local prev_type = nil
	local count = 0

	for i, line in ipairs(lines) do
		local log_type = LogAnalyzer.extract_log_type(line)

		if log_type == prev_type then
			count = count + 1
			if count >= threshold then
				table.insert(loops, {
					line_num = i - threshold,
					log_type = log_type,
					count = count,
					content = LogAnalyzer.strip_ansi(line),
				})
			end
		else
			count = 1
			prev_type = log_type
		end
	end

	return loops
end

function LogAnalyzer.analyze_interrupts(lines)
	local interrupts = {}
	local vectors = {}

	for i, line in ipairs(lines) do
		local vector = line:match("vector=(%d+)")
		if vector then
			vector = tonumber(vector)
			vectors[vector] = (vectors[vector] or 0) + 1
			table.insert(interrupts, {
				line_num = i,
				vector = vector,
				content = LogAnalyzer.strip_ansi(line),
			})
		end
	end

	return {
		total = #interrupts,
		vectors = vectors,
		interrupts = interrupts,
	}
end

function LogAnalyzer.output_errors(errors, output_dir)
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

	LogAnalyzer.write_file(output_dir .. "/01_errors.txt", content)
	print("✓ Generated: 01_errors.txt (" .. #errors .. " errors)")
end

function LogAnalyzer.output_warnings(warnings, output_dir)
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

	LogAnalyzer.write_file(output_dir .. "/02_warnings.txt", content)
	print("✓ Generated: 02_warnings.txt (" .. #warnings .. " warnings)")
end

function LogAnalyzer.output_smm(smm_data, output_dir)
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

	LogAnalyzer.write_file(output_dir .. "/03_smm_transitions.txt", content)
	print("✓ Generated: 03_smm_transitions.txt (" .. smm_data.enter .. " transitions)")
end

function LogAnalyzer.output_leaks(leaks, output_dir)
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

	LogAnalyzer.write_file(output_dir .. "/04_memory_leaks.txt", content)
	print("✓ Generated: 04_memory_leaks.txt (" .. #leaks .. " leaks)")
end

function LogAnalyzer.output_subsystems(subsystems, output_dir)
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
	content = content .. string.format("%-25s %-15s %-10s %s\n", "SUBSYSTEM", "INITIALIZED", "SUCCESS", "LINES")
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

		content = content .. string.format("%-25s %-15s %-10s %s\n", item.name, symbol_init, symbol_success, line_range)
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

	LogAnalyzer.write_file(output_dir .. "/05_subsystems.txt", content)
	print("✓ Generated: 05_subsystems.txt")
end

function LogAnalyzer.output_statistics(log_types, total_lines, output_dir)
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

	content = content .. string.format("%-5s %-30s %-10s %s\n", "RANK", "LOG TYPE", "COUNT", "PERCENTAGE")
	content = content .. string.rep("-", 80) .. "\n"

	for i, item in ipairs(sorted) do
		local percentage = (item.count / total_lines) * 100
		content = content
			.. string.format("%-5d %-30s %-10d %.2f%%\n", i, "[" .. item.name .. "]", item.count, percentage)
	end

	LogAnalyzer.write_file(output_dir .. "/06_statistics.txt", content)
	print("✓ Generated: 06_statistics.txt (" .. #sorted .. " log types)")
end

function LogAnalyzer.output_interrupts(int_data, output_dir)
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
		content = content .. string.format("%-10s %-15s %s\n", "VECTOR", "COUNT", "DESCRIPTION")
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
			content = content .. string.format("%-10d %-15d %s\n", item.vector, item.count, desc)
		end
	else
		content = content .. "No interrupt activity detected.\n"
	end

	LogAnalyzer.write_file(output_dir .. "/07_interrupts.txt", content)
	print("✓ Generated: 07_interrupts.txt (" .. int_data.total .. " events)")
end

function LogAnalyzer.output_last_lines(last_lines, output_dir)
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

	LogAnalyzer.write_file(output_dir .. "/08_last_lines.txt", content)
	print("✓ Generated: 08_last_lines.txt")
end

function LogAnalyzer.output_summary(data, output_dir)
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

	LogAnalyzer.write_file(output_dir .. "/00_SUMMARY.txt", content)
	print("✓ Generated: 00_SUMMARY.txt")
end

function LogAnalyzer.analyze_all(filename, output_dir)
	print("\n" .. string.rep("=", 80))
	print("                    FKERNEL LOG ANALYZER")
	print(string.rep("=", 80) .. "\n")

	LogAnalyzer.ensure_directory(output_dir)
	print("✓ Output directory: " .. output_dir .. "\n")

	print("Reading log file...")
	local lines = LogAnalyzer.read_log(filename)
	if not lines then
		return false
	end

	print("\nAnalyzing log data...\n")

	local errors = LogAnalyzer.find_errors(lines)
	local warnings = LogAnalyzer.find_warnings(lines)
	local smm_data = LogAnalyzer.count_smm_transitions(lines)
	local leaks = LogAnalyzer.find_memory_leaks(lines)
	local subsystems = LogAnalyzer.check_subsystem_init(lines)
	local log_types = LogAnalyzer.count_log_types(lines)
	local int_data = LogAnalyzer.analyze_interrupts(lines)
	local last_lines = LogAnalyzer.get_last_lines(lines, 100)

	print("Generating reports...\n")

	LogAnalyzer.output_errors(errors, output_dir)
	LogAnalyzer.output_warnings(warnings, output_dir)
	LogAnalyzer.output_smm(smm_data, output_dir)
	LogAnalyzer.output_leaks(leaks, output_dir)
	LogAnalyzer.output_subsystems(subsystems, output_dir)
	LogAnalyzer.output_statistics(log_types, #lines, output_dir)
	LogAnalyzer.output_interrupts(int_data, output_dir)
	LogAnalyzer.output_last_lines(last_lines, output_dir)

	LogAnalyzer.output_summary({
		filename = filename,
		total_lines = #lines,
		errors_count = #errors,
		warnings_count = #warnings,
		leaks_count = #leaks,
		smm_enter = smm_data.enter,
		smm_exit = smm_data.exit,
		subsystems = subsystems,
		log_types = log_types,
	}, output_dir)

	print("\n" .. string.rep("=", 80))
	print("Analysis complete! Reports saved to: " .. output_dir)
	print(string.rep("=", 80) .. "\n")

	print("QUICK SUMMARY:")
	print("  Total lines:     " .. #lines)
	print("  Errors:          " .. #errors)
	print("  Warnings:        " .. #warnings)
	print("  Memory leaks:    " .. #leaks)
	print("  SMM transitions: " .. smm_data.enter .. " enter, " .. smm_data.exit .. " exit")
	print("\nCheck 00_SUMMARY.txt for detailed analysis.\n")

	return true
end

function Main()
	print([[
    ███████╗██╗  ██╗███████╗██████╗ ███╗   ██╗███████╗██╗     
    ██╔════╝██║ ██╔╝██╔════╝██╔══██╗████╗  ██║██╔════╝██║     
    █████╗  █████╔╝ █████╗  ██████╔╝██╔██╗ ██║█████╗  ██║     
    ██╔══╝  ██╔═██╗ ██╔══╝  ██╔══██╗██║╚██╗██║██╔══╝  ██║     
    ██║     ██║  ██╗███████╗██║  ██║██║ ╚████║███████╗███████╗
    ╚═╝     ╚═╝  ╚═╝╚══════╝╚═╝  ╚═╝╚═╝  ╚═══╝╚══════╝╚══════╝
    
    LOG ANALYZER v2.0 - Automated Analysis
    ]])

	local filename = arg[1] or "logs/interrupt_logs.log"
	local output_dir = arg[2] or "logs/analysis"

	local success = LogAnalyzer.analyze_all(filename, output_dir)

	if not success then
		os.exit(1)
	end
end

Main()
