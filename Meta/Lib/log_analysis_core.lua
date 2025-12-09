#!/usr/bin/env lua

local LogParser = require("Meta.Lib.log_parser")

local LogAnalysisCore = {}

function LogAnalysisCore.find_errors(lines)
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
					content = LogParser.strip_ansi(line),
					pattern = pattern,
					severity = LogParser.detect_severity(line),
				})
				break
			end
		end
	end

	return errors
end

function LogAnalysisCore.find_warnings(lines)
	local warnings = {}

	for i, line in ipairs(lines) do
		local severity = LogParser.detect_severity(line)
		if severity == "WARNING" or line:lower():find("warning") then
			table.insert(warnings, {
				line_num = i,
				content = LogParser.strip_ansi(line),
			})
		end
	end

	return warnings
end

function LogAnalysisCore.count_smm_transitions(lines)
	local smm_enter = 0
	local smm_exit = 0
	local transitions = {}

	for i, line in ipairs(lines) do
		if line:find("SMM: enter") then
			smm_enter = smm_enter + 1
			table.insert(transitions, {
				line_num = i,
				type = "enter",
				content = LogParser.strip_ansi(line),
			})
		elseif line:find("SMM: after RSM") then
			smm_exit = smm_exit + 1
			table.insert(transitions, {
				line_num = i,
				type = "exit",
				content = LogParser.strip_ansi(line),
			})
		end
	end

	return {
		enter = smm_enter,
		exit = smm_exit,
		transitions = transitions,
	}
end

function LogAnalysisCore.count_log_types(lines)
	local counts = {}

	for _, line in ipairs(lines) do
		local log_type = LogParser.extract_log_type(line)
		if log_type then
			counts[log_type] = (counts[log_type] or 0) + 1
		end
	end

	return counts
end

function LogAnalysisCore.find_memory_leaks(lines)
	local leaks = {}

	for i, line in ipairs(lines) do
		if line:find("leaked ptr") or line:find("leak") then
			local ptr = line:match("ptr=(0x%x+)")
			table.insert(leaks, {
				line_num = i,
				ptr = ptr,
				content = LogParser.strip_ansi(line),
			})
		end
	end

	return leaks
end

function LogAnalysisCore.check_subsystem_init(lines)
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
					content = LogParser.strip_ansi(line),
				})

				if line:find("initialized") or line:find("complete") or line:find("enabled") or line:find("set to") then
					local severity = LogParser.detect_severity(line)
					if severity == "SUCCESS" or line:match("%[32m") then
						status[subsystem].success = true
					end
				end
			end
		end
	end

	return status
end

function LogAnalysisCore.search_pattern(lines, pattern, case_sensitive)
	local results = {}

	for i, line in ipairs(lines) do
		local search_line = case_sensitive and line or line:lower()
		local search_pattern = case_sensitive and pattern or pattern:lower()

		if search_line:find(search_pattern) then
			table.insert(results, {
				line_num = i,
				content = LogParser.strip_ansi(line),
			})
		end
	end

	return results
end

function LogAnalysisCore.get_last_lines(lines, n)
	local result = {}
	local start = math.max(1, #lines - n + 1)

	for i = start, #lines do
		table.insert(result, {
			line_num = i,
			content = LogParser.strip_ansi(lines[i]),
		})
	end

	return result
end

function LogAnalysisCore.detect_loops(lines, threshold)
	threshold = threshold or 10
	local consecutive = {}
	local loops = {}
	local prev_type = nil
	local count = 0

	for i, line in ipairs(lines) do
		local log_type = LogParser.extract_log_type(line)

		if log_type == prev_type then
			count = count + 1
			if count >= threshold then
				table.insert(loops, {
					line_num = i - threshold,
					log_type = log_type,
					count = count,
					content = LogParser.strip_ansi(line),
				})
			end
		else
			count = 1
			prev_type = log_type
		end
	end

	return loops
end

function LogAnalysisCore.analyze_interrupts(lines)
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
				content = LogParser.strip_ansi(line),
			})
		end
	end

	return {
		total = #interrupts,
		vectors = vectors,
		interrupts = interrupts,
	}
end

return LogAnalysisCore